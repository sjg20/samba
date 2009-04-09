/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 * Nick Bane modifications flagged NCB
 * Endian handling patches by James Ng.
 * mkyaffs2image hacks by NCB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * makeyaffs2image.c 
 *
 * Makes a YAFFS2 file system image that can be used to load up a file system.
 * Uses default Linux MTD layout - change if you need something different.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#define __user
#include <mtd/mtd-user.h>
#include "yaffs_ecc.h"
#include "yaffs_guts.h"

#include "yaffs_packedtags2.h"

unsigned yaffs_traceMask=0;

#define MAX_OBJECTS 10000

#define chunkSize 2048
#define spareSize 64
#define PT2_BYTES 25

const char * mkyaffsimage_c_version = "$Id: mkyaffs2image.c,v 1.4 2007-02-14 01:09:06 wookey Exp $";

#if 1
#define dbg(a,b...) do {} while (0)
#else
#define dbg(a,b...) printf(a,b...)
#endif

 
static int	layout_no;

static struct nand_oobinfo oob_layout[] = {
    /* KSI:
     * Dummy "raw" layout - no ECC, all the bytes are free. Does NOT
     * really work, only used for compatibility with CVS YAFFS2 that
     * never ever worked with any stock MTD.
     */
    {
        .useecc = MTD_NANDECC_AUTOPLACE,
        .eccbytes = 0,
        .eccpos = {},
        .oobfree = { {0, 64} }
    },
    /* KSI:
     * Regular MTD AUTOPLACED ECC for large page NAND devices, the
     * only one existing in stock MTD so far. It corresponds to layout# 1
     * in command line arguments. Any other layouts could be added to
     * the list when they made their way in kernel's MTD. The structure
     * is simply copied from kernel's drivers/mtd/nand/nand_base.c as-is.
     */
    {
        .useecc = MTD_NANDECC_AUTOPLACE,
        .eccbytes = 24,
        .eccpos = {
            40, 41, 42, 43, 44, 45, 46, 47,
            48, 49, 50, 51, 52, 53, 54, 55,
            56, 57, 58, 59, 60, 61, 62, 63},
        .oobfree = { {2, 38} }
    },
    /* End-of-list marker */
    {
        .useecc = -1,
    }
};

typedef struct {
	dev_t dev;
	ino_t ino;
	int   obj;
} objItem;


static objItem obj_list[MAX_OBJECTS];
static int n_obj = 0;
static int obj_id = YAFFS_NOBJECT_BUCKETS + 1;

static int nObjects = 0, nDirectories = 0, nPages = 0;

static int outFile;

static int error;

static int convert_endian = 0;

static int obj_compare(const void *a, const void * b)
{
    objItem *oa, *ob;

    oa = (objItem *)a;
    ob = (objItem *)b;

    if(oa->dev < ob->dev) return -1;
    if(oa->dev > ob->dev) return 1;
    if(oa->ino < ob->ino) return -1;
    if(oa->ino > ob->ino) return 1;

    return 0;
}


static void add_obj_to_list(dev_t dev, ino_t ino, int obj)
{
	if(n_obj < MAX_OBJECTS)
	{
		obj_list[n_obj].dev = dev;
		obj_list[n_obj].ino = ino;
		obj_list[n_obj].obj = obj;
		n_obj++;
		qsort(obj_list,n_obj,sizeof(objItem),obj_compare);
		
	}
	else
	{
		// oops! not enough space in the object array
		fprintf(stderr,"Not enough space in object array\n");
		exit(2);
	}
}


static int find_obj_in_list(dev_t dev, ino_t ino)
{
	objItem *i = NULL;
	objItem test;

	test.dev = dev;
	test.ino = ino;
	
	if(n_obj > 0)
	{
		i = bsearch(&test,obj_list,n_obj,sizeof(objItem),obj_compare);
	}

	if(i)
	{
		return i->obj;
	}
	return -1;
}

static void yaffs_PutDataECC(const __u8 * data, unsigned char *oob_buf)
{
    unsigned char ecc_code[3];
    int eccsteps = chunkSize / 256;
    int eccidx = 0, datidx = 0, i;
    struct nand_oobinfo *oobsel;
    __u32 *oob_config;

    oobsel = &oob_layout[layout_no];
    oob_config = oobsel->eccpos;

    for (; eccsteps; eccsteps--) {
        yaffs_ECCCalculate (&data[datidx], ecc_code);
	for (i = 0; i < 3; i++, eccidx++)
	    oob_buf[oob_config[eccidx]] = ecc_code[i];
	datidx += 256;
    }
}


/* KSI:
 * No big endian for now. This is left for a later time. The existing code
 * is FUBAR.
 */
#if 0
/* This little function converts a little endian tag to a big endian tag.
 * NOTE: The tag is not usable after this other than calculating the CRC
 * with.
 */
static void little_to_big_endian(yaffs_Tags *tagsPtr)
{
#if 0 // FIXME NCB
    yaffs_TagsUnion * tags = (yaffs_TagsUnion* )tagsPtr; // Work in bytes.
    yaffs_TagsUnion   temp;

    memset(&temp, 0, sizeof(temp));
    // Ick, I hate magic numbers.
    temp.asBytes[0] = ((tags->asBytes[2] & 0x0F) << 4) | ((tags->asBytes[1] & 0xF0) >> 4);
    temp.asBytes[1] = ((tags->asBytes[1] & 0x0F) << 4) | ((tags->asBytes[0] & 0xF0) >> 4);
    temp.asBytes[2] = ((tags->asBytes[0] & 0x0F) << 4) | ((tags->asBytes[2] & 0x30) >> 2) | ((tags->asBytes[3] & 0xC0) >> 6);
    temp.asBytes[3] = ((tags->asBytes[3] & 0x3F) << 2) | ((tags->asBytes[2] & 0xC0) >> 6);
    temp.asBytes[4] = ((tags->asBytes[6] & 0x03) << 6) | ((tags->asBytes[5] & 0xFC) >> 2);
    temp.asBytes[5] = ((tags->asBytes[5] & 0x03) << 6) | ((tags->asBytes[4] & 0xFC) >> 2);
    temp.asBytes[6] = ((tags->asBytes[4] & 0x03) << 6) | (tags->asBytes[7] & 0x3F);
    temp.asBytes[7] = (tags->asBytes[6] & 0xFC) | ((tags->asBytes[7] & 0xC0) >> 6);

    // Now copy it back.
    tags->asBytes[0] = temp.asBytes[0];
    tags->asBytes[1] = temp.asBytes[1];
    tags->asBytes[2] = temp.asBytes[2];
    tags->asBytes[3] = temp.asBytes[3];
    tags->asBytes[4] = temp.asBytes[4];
    tags->asBytes[5] = temp.asBytes[5];
    tags->asBytes[6] = temp.asBytes[6];
    tags->asBytes[7] = temp.asBytes[7];
#endif
}
#endif

static void nandmtd2_pt2buf(unsigned char *buf, yaffs_PackedTags2 *pt)
{
	int		i, j = 0, k, n;
	unsigned char	pt2_byte_buf[PT2_BYTES];
	
	*((unsigned int *) &pt2_byte_buf[0]) = pt->t.sequenceNumber;
	*((unsigned int *) &pt2_byte_buf[4]) = pt->t.objectId;
	*((unsigned int *) &pt2_byte_buf[8]) = pt->t.chunkId;
	*((unsigned int *) &pt2_byte_buf[12]) = pt->t.byteCount;
	pt2_byte_buf[16] = pt->ecc.colParity;
	pt2_byte_buf[17] = pt->ecc.lineParity & 0xff;
	pt2_byte_buf[18] = (pt->ecc.lineParity >> 8) & 0xff;
	pt2_byte_buf[19] = (pt->ecc.lineParity >> 16) & 0xff;
	pt2_byte_buf[20] = (pt->ecc.lineParity >> 24) & 0xff;
	pt2_byte_buf[21] = pt->ecc.lineParityPrime & 0xff;
	pt2_byte_buf[22] = (pt->ecc.lineParityPrime >> 8) & 0xff;
	pt2_byte_buf[23] = (pt->ecc.lineParityPrime >> 16) & 0xff;
	pt2_byte_buf[24] = (pt->ecc.lineParityPrime >> 24) & 0xff;

	k = oob_layout[layout_no].oobfree[j][0];
	n = oob_layout[layout_no].oobfree[j][1];
		
	if (n == 0) {
		fprintf(stderr, "No OOB space for tags");
		exit(-1);
	}
                                
	for (i = 0; i < PT2_BYTES; i++) {
		if (n == 0) {
			j++;
			k = oob_layout[layout_no].oobfree[j][0];
			n = oob_layout[layout_no].oobfree[j][1];
			if (n == 0) {
				fprintf(stderr, "No OOB space for tags");
				exit(-1);
			}
		}
		buf[k++] = pt2_byte_buf[i];
		n--;
	}
}

static int write_chunk(__u8 *data, __u32 objId, __u32 chunkId, __u32 nBytes)
{
	yaffs_ExtendedTags t;
	yaffs_PackedTags2 pt;
	unsigned char	spare_buf[spareSize];


	error = write(outFile,data,chunkSize);
	if(error < 0) return error;

	yaffs_InitialiseTags(&t);
	
	t.chunkId = chunkId;
//	t.serialNumber = 0;
	t.serialNumber = 1;	// **CHECK**
	t.byteCount = nBytes;
	t.objectId = objId;
	
	t.sequenceNumber = YAFFS_LOWEST_SEQUENCE_NUMBER;

// added NCB **CHECK**
	t.chunkUsed = 1;

/* KSI: Broken anyway -- e.g. &t is pointer to a wrong type... */
#if 0
	if (convert_endian)
	{
    	    little_to_big_endian(&t);
	}
#endif

	yaffs_PackTags2(&pt,&t);

        memset(spare_buf, 0xff, sizeof(spare_buf));
        
        if (layout_no == 0) {
                memcpy(spare_buf, &pt, sizeof(yaffs_PackedTags2));
        } else {
                nandmtd2_pt2buf(spare_buf, &pt);
        }	

    	yaffs_PutDataECC(data, &spare_buf[0]);

	nPages++;

	return write(outFile,spare_buf,spareSize);
}

#define SWAP32(x)   ((((x) & 0x000000FF) << 24) | \
                     (((x) & 0x0000FF00) << 8 ) | \
                     (((x) & 0x00FF0000) >> 8 ) | \
                     (((x) & 0xFF000000) >> 24))

#define SWAP16(x)   ((((x) & 0x00FF) << 8) | \
                     (((x) & 0xFF00) >> 8))
        
/* KSI: Removed for now. TBD later when the proper util (from scratch) is written */
#if 0
// This one is easier, since the types are more standard. No funky shifts here.
static void object_header_little_to_big_endian(yaffs_ObjectHeader* oh)
{
    oh->type = SWAP32(oh->type); // GCC makes enums 32 bits.
    oh->parentObjectId = SWAP32(oh->parentObjectId); // int
    oh->sum__NoLongerUsed = SWAP16(oh->sum__NoLongerUsed); // __u16 - Not used, but done for completeness.
    // name = skip. Char array. Not swapped.
    oh->yst_mode = SWAP32(oh->yst_mode);
#ifdef CONFIG_YAFFS_WINCE // WinCE doesn't implement this, but we need to just in case. 
    // In fact, WinCE would be *THE* place where this would be an issue!
    oh->notForWinCE[0] = SWAP32(oh->notForWinCE[0]);
    oh->notForWinCE[1] = SWAP32(oh->notForWinCE[1]);
    oh->notForWinCE[2] = SWAP32(oh->notForWinCE[2]);
    oh->notForWinCE[3] = SWAP32(oh->notForWinCE[3]);
    oh->notForWinCE[4] = SWAP32(oh->notForWinCE[4]);
#else
    // Regular POSIX.
    oh->yst_uid = SWAP32(oh->yst_uid);
    oh->yst_gid = SWAP32(oh->yst_gid);
    oh->yst_atime = SWAP32(oh->yst_atime);
    oh->yst_mtime = SWAP32(oh->yst_mtime);
    oh->yst_ctime = SWAP32(oh->yst_ctime);
#endif

    oh->fileSize = SWAP32(oh->fileSize); // Aiee. An int... signed, at that!
    oh->equivalentObjectId = SWAP32(oh->equivalentObjectId);
    // alias  - char array.
    oh->yst_rdev = SWAP32(oh->yst_rdev);

#ifdef CONFIG_YAFFS_WINCE
    oh->win_ctime[0] = SWAP32(oh->win_ctime[0]);
    oh->win_ctime[1] = SWAP32(oh->win_ctime[1]);
    oh->win_atime[0] = SWAP32(oh->win_atime[0]);
    oh->win_atime[1] = SWAP32(oh->win_atime[1]);
    oh->win_mtime[0] = SWAP32(oh->win_mtime[0]);
    oh->win_mtime[1] = SWAP32(oh->win_mtime[1]);
    oh->roomToGrow[0] = SWAP32(oh->roomToGrow[0]);
    oh->roomToGrow[1] = SWAP32(oh->roomToGrow[1]);
    oh->roomToGrow[2] = SWAP32(oh->roomToGrow[2]);
    oh->roomToGrow[3] = SWAP32(oh->roomToGrow[3]);
    oh->roomToGrow[4] = SWAP32(oh->roomToGrow[4]);
    oh->roomToGrow[5] = SWAP32(oh->roomToGrow[5]);
#else
    oh->roomToGrow[0] = SWAP32(oh->roomToGrow[0]);
    oh->roomToGrow[1] = SWAP32(oh->roomToGrow[1]);
    oh->roomToGrow[2] = SWAP32(oh->roomToGrow[2]);
    oh->roomToGrow[3] = SWAP32(oh->roomToGrow[3]);
    oh->roomToGrow[4] = SWAP32(oh->roomToGrow[4]);
    oh->roomToGrow[5] = SWAP32(oh->roomToGrow[5]);
    oh->roomToGrow[6] = SWAP32(oh->roomToGrow[6]);
    oh->roomToGrow[7] = SWAP32(oh->roomToGrow[7]);
    oh->roomToGrow[8] = SWAP32(oh->roomToGrow[8]);
    oh->roomToGrow[9] = SWAP32(oh->roomToGrow[9]);
    oh->roomToGrow[10] = SWAP32(oh->roomToGrow[10]);
    oh->roomToGrow[11] = SWAP32(oh->roomToGrow[11]);
#endif
}
#endif

static int write_object_header(int objId, yaffs_ObjectType t, struct stat *s, int parent, const char *name, int equivalentObj, const char * alias)
{
	__u8 bytes[chunkSize];
	
	
	yaffs_ObjectHeader *oh = (yaffs_ObjectHeader *)bytes;
	
	memset(bytes,0xff,sizeof(bytes));
	
	oh->type = t;

	oh->parentObjectId = parent;
	
	strncpy(oh->name,name,YAFFS_MAX_NAME_LENGTH);
	
	
	if(t != YAFFS_OBJECT_TYPE_HARDLINK)
	{
		oh->yst_mode = s->st_mode;
		oh->yst_uid = s->st_uid;
// NCB 12/9/02		oh->yst_gid = s->yst_uid;
		oh->yst_gid = s->st_gid;
		oh->yst_atime = s->st_atime;
		oh->yst_mtime = s->st_mtime;
		oh->yst_ctime = s->st_ctime;
		oh->yst_rdev  = s->st_rdev;
	}
	
	if(t == YAFFS_OBJECT_TYPE_FILE)
	{
		oh->fileSize = s->st_size;
	}
	
	if(t == YAFFS_OBJECT_TYPE_HARDLINK)
	{
		oh->equivalentObjectId = equivalentObj;
	}
	
	if(t == YAFFS_OBJECT_TYPE_SYMLINK)
	{
		strncpy(oh->alias,alias,YAFFS_MAX_ALIAS_LENGTH);
	}

/* KSI: FUBAR. Left for a leter time. */
#if 0
	if (convert_endian)
	{
    		object_header_little_to_big_endian(oh);
	}
#endif
	
	return write_chunk(bytes,objId,0,0xffff);
	
}


static int process_directory(int parent, const char *path)
{

	DIR *dir;
	struct dirent *entry;

	nDirectories++;
	
	dir = opendir(path);

	if(dir)
	{
		while((entry = readdir(dir)) != NULL)
		{
		
			/* Ignore . and .. */
			if(strcmp(entry->d_name,".") &&
			   strcmp(entry->d_name,".."))
 			{
 				char full_name[500];
				struct stat stats;
				int equivalentObj;
				int newObj;
				
				sprintf(full_name,"%s/%s",path,entry->d_name);
				
				lstat(full_name,&stats);
				
				if(S_ISLNK(stats.st_mode) ||
				    S_ISREG(stats.st_mode) ||
				    S_ISDIR(stats.st_mode) ||
				    S_ISFIFO(stats.st_mode) ||
				    S_ISBLK(stats.st_mode) ||
				    S_ISCHR(stats.st_mode) ||
				    S_ISSOCK(stats.st_mode))
				{
				
					newObj = obj_id++;
					nObjects++;
					
					dbg("Object %d, %s is a ",newObj,full_name);
					
					/* We're going to create an object for it */
					if((equivalentObj = find_obj_in_list(stats.st_dev, stats.st_ino)) > 0)
					{
					 	/* we need to make a hard link */
					 	dbg("hard link to object %d\n",equivalentObj);
						error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_HARDLINK, &stats, parent, entry->d_name, equivalentObj, NULL);
					}
					else 
					{
						
						add_obj_to_list(stats.st_dev,stats.st_ino,newObj);
						
						if(S_ISLNK(stats.st_mode))
						{
					
							char symname[500];
						
							memset(symname,0, sizeof(symname));
					
							readlink(full_name,symname,sizeof(symname) -1);
						
							dbg("symlink to \"%s\"\n",symname);
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SYMLINK, &stats, parent, entry->d_name, -1, symname);

						}
						else if(S_ISREG(stats.st_mode))
						{
							dbg("file, ");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_FILE, &stats, parent, entry->d_name, -1, NULL);

							if(error >= 0)
							{
								int h;
								__u8 bytes[chunkSize];
								int nBytes;
								int chunk = 0;
								
								h = open(full_name,O_RDONLY);
								if(h >= 0)
								{
									memset(bytes,0xff,sizeof(bytes));
									while((nBytes = read(h,bytes,sizeof(bytes))) > 0)
									{
										chunk++;
										write_chunk(bytes,newObj,chunk,nBytes);
										memset(bytes,0xff,sizeof(bytes));
									}
									if(nBytes < 0) 
									   error = nBytes;
									   
									dbg("%d data chunks written\n",chunk);
        								close(h);
								}
								else
								{
									perror("Error opening file");
								}
								
							}							
														
						}
						else if(S_ISSOCK(stats.st_mode))
						{
							dbg("socket\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISFIFO(stats.st_mode))
						{
							dbg("fifo\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISCHR(stats.st_mode))
						{
							dbg("character device\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISBLK(stats.st_mode))
						{
							dbg("block device\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_SPECIAL, &stats, parent, entry->d_name, -1, NULL);
						}
						else if(S_ISDIR(stats.st_mode))
						{
							dbg("directory\n");
							error =  write_object_header(newObj, YAFFS_OBJECT_TYPE_DIRECTORY, &stats, parent, entry->d_name, -1, NULL);
							process_directory(newObj,full_name);
						}
					}
				}
				else
				{
					dbg(" we don't handle this type\n");
				}
			}
		}
		closedir(dir);
	}
	
	return 0;

}

int mkyaffs2_create (const char *directory, const char *output)
{
	struct stat stats;
	if(stat(directory,&stats) < 0)
	{
		printf("Could not stat %s\n",directory);
                return -1;
	}
	
	if(!S_ISDIR(stats.st_mode))
	{
		printf(" %s is not a directory\n",directory);
                return -1;
	}
	
	outFile = open(output,O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
	
	
	if(outFile < 0)
	{
		printf("Could not open output file %s\n",output);
                return -1;
	}
	
	dbg("Processing directory %s into image file %s\n",directory, output);
	error =  write_object_header(1, YAFFS_OBJECT_TYPE_DIRECTORY, &stats, 1,"", -1, NULL);

	if(error)
	error = process_directory(YAFFS_OBJECTID_ROOT,directory);
	
	close(outFile);

        return 0;
}
	
#if 0
void usage(void)
{
	printf("usage: mkyaffs2image layout# dir image_file [convert]\n");
	printf("	layout#    NAND OOB layout # (0 - raw, 1 - nand_oob_64)\n");
	printf("	dir        the directory tree to be converted\n");
	printf("	image_file the output file to hold the image\n");
	printf("	'convert'  make a big-endian img on a little-endian machine. BROKEN !\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	struct stat stats;
	int	i;
	
	printf("mkyaffs2image: image building tool for YAFFS2 built "__DATE__"\n");
	
	if ((argc < 4) || (sscanf(argv[1], "%u", &layout_no) != 1))
	{
	        usage();
	}

	i = 0;
	
	while (oob_layout[i].useecc != -1)
	        i++;
	        
        if (layout_no >= i)
                usage();

	if ((argc == 5) && (!strncmp(argv[4], "convert", strlen("convert"))))
	{
                fprintf (stderr, "WARNING: ENDIAN CONVERSION IS BROKEN\n");
	        /* KSI: Broken as of now. TBD. Fail. */
	        usage();
                convert_endian = 1;
        }
    
	if(stat(argv[2],&stats) < 0)
	{
		printf("Could not stat %s\n",argv[2]);
		exit(1);
	}
	
	if(!S_ISDIR(stats.st_mode))
	{
		printf(" %s is not a directory\n",argv[2]);
		exit(1);
	}
	
	outFile = open(argv[3],O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
	
	
	if(outFile < 0)
	{
		printf("Could not open output file %s\n",argv[3]);
		exit(1);
	}
	
	printf("Processing directory %s into image file %s\n",argv[2],argv[3]);
	error =  write_object_header(1, YAFFS_OBJECT_TYPE_DIRECTORY, &stats, 1,"", -1, NULL);

	if(error)
	error = process_directory(YAFFS_OBJECTID_ROOT,argv[2]);
	
	close(outFile);
	
	if(error < 0)
	{
		perror("operation incomplete");
		exit(1);
	}
	else
	{
		printf("Operation complete.\n"
		       "%d objects in %d directories\n"
		       "%d NAND pages\n",nObjects, nDirectories, nPages);
	}
	
	close(outFile);
	
	exit(0);
}	
#endif
