#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "at91sam.h"
#include "nand.h"
#include "common.h"

#define BOOL int
#define TRUE 1
#define FALSE 0

/** Settings from sn9260.h in bootstrap */
#define AT91C_SM_NWE_SETUP      (1 << 0)
#define AT91C_SM_NCS_WR_SETUP   (1 << 8)
#define AT91C_SM_NRD_SETUP      (1 << 16)
#define AT91C_SM_NCS_RD_SETUP   (1 << 24)

#define AT91C_SM_NWE_PULSE      (7 << 0)
#define AT91C_SM_NCS_WR_PULSE   (9 << 8)
#define AT91C_SM_NRD_PULSE      (6 << 16)
#define AT91C_SM_NCS_RD_PULSE   (8 << 24)

#define AT91C_SM_NWE_CYCLE      (10 << 0)
#define AT91C_SM_NRD_CYCLE      (9 << 16)
#define AT91C_SM_TDF            (1 << 16)

#define AT91C_SMARTMEDIA_BASE   0x40000000

#define AT91_SMART_MEDIA_ALE    (1 << 21)       /* our ALE is AD21 */
#define AT91_SMART_MEDIA_CLE    (1 << 22)       /* our CLE is AD22 */

#define NAND_DISABLE_CE() do { writel(AT91C_PIO_PC14, AT91C_PIOC_SODR); } while (0);
#define NAND_ENABLE_CE() do { writel(AT91C_PIO_PC14, AT91C_PIOC_CODR); } while(0);

#define NAND_WAIT_READY() while (!(readl(AT91C_PIOC_PDSR) & AT91C_PIO_PC13))

#define WRITE_NAND_COMMAND(d) do{ at91_write_byte(at91, AT91C_SMARTMEDIA_BASE | AT91_SMART_MEDIA_CLE, d); } while (0)
#define WRITE_NAND_ADDRESS(d) do{ at91_write_byte(at91, AT91C_SMARTMEDIA_BASE | AT91_SMART_MEDIA_ALE, d); } while (0)
#define WRITE_NAND(d) do{ at91_write_byte(at91, AT91C_SMARTMEDIA_BASE, d); } while (0)
#define READ_NAND() at91_read_byte(at91, AT91C_SMARTMEDIA_BASE)
#define WRITE_NAND_BLOCK(d,l) at91_write_data (at91, AT91C_SMARTMEDIA_BASE, d, l)

#define nand_16bit(a) 0

#define NAND_PAGE_SIZE 2048
#define NAND_OOB_SIZE 64

#define NAND_BLOCK_SIZE (128 * 1024)
#define PAGES_PER_BLOCK (NAND_BLOCK_SIZE / NAND_PAGE_SIZE)
#define MAX_BLOCKS ((4UL * 1024 * 1024) / (NAND_BLOCK_SIZE / 1024)) // 4GB max

#define ADDR2BLOCK(a) ((a) / NAND_BLOCK_SIZE)

static char bbt[MAX_BLOCKS];

int nand_init (at91_t *at91)
{
    memset(bbt, -1, MAX_BLOCKS);
    /* Code taken from Bootstrap v1.9 */
    writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) | AT91C_EBI_CS3A_SM, AT91C_BASE_CCFG + CCFG_EBICSA);

    /* Configure SMC CS3 */
    writel((AT91C_SM_NWE_SETUP | AT91C_SM_NCS_WR_SETUP | AT91C_SM_NRD_SETUP | AT91C_SM_NCS_RD_SETUP), AT91C_BASE_SMC + SMC_SETUP3);
    writel((AT91C_SM_NWE_PULSE | AT91C_SM_NCS_WR_PULSE | AT91C_SM_NRD_PULSE | AT91C_SM_NCS_RD_PULSE), AT91C_BASE_SMC + SMC_PULSE3);
    writel((AT91C_SM_NWE_CYCLE | AT91C_SM_NRD_CYCLE) , AT91C_BASE_SMC + SMC_CYCLE3);
    writel((AT91C_SMC_READMODE | AT91C_SMC_WRITEMODE | AT91C_SMC_NWAITM_NWAIT_DISABLE |
                AT91C_SMC_DBW_WIDTH_EIGTH_BITS | AT91C_SM_TDF), AT91C_BASE_SMC + SMC_CTRL3);

    /* Configure the PIO controller */
    writel((1 << AT91C_ID_PIOC), PMC_PCER + AT91C_BASE_PMC);
    writel((1 << AT91C_ID_SYS), PMC_PCER + AT91C_BASE_PMC);

    /* Configure PIOs */
    writel((1 << 13), AT91C_BASE_PIOC + PIO_IDR(0));
    writel((1 << 13), AT91C_BASE_PIOC + PIO_PPUER(0));
    writel((1 << 13), AT91C_BASE_PIOC + PIO_ODR(0));
    writel((1 << 13), AT91C_BASE_PIOC + PIO_PER(0));

    writel((1 << 14), AT91C_BASE_PIOC + PIO_OER(0));
    writel((1 << 14), AT91C_BASE_PIOC + PIO_PER(0));

    /* Configure SMC in 8 bits mode */
    writel((readl(AT91C_BASE_SMC + SMC_CTRL3) & ~(AT91C_SMC_DBW)) | AT91C_SMC_DBW_WIDTH_EIGTH_BITS, AT91C_BASE_SMC + SMC_CTRL3);

    return 0;
}

int nand_read_id (at91_t *at91)
{
    int manu, device;

    /* Enable chipset */
    NAND_ENABLE_CE();

    /* Ask the Nand its IDs */
    WRITE_NAND_COMMAND(CMD_READID);
    WRITE_NAND_ADDRESS(0x00);

    /* Read answer */
    manu = READ_NAND();
    device       = READ_NAND();

    /* Disable chipset before returning */
    NAND_DISABLE_CE();
    //printf ("manu: 0x%x device: 0x%x\n", manu, device);

    return manu << 8 | device;
}

static int nand_read_page (at91_t *at91, unsigned int uSectorAddr, char *pOutBuffer, unsigned int fZone)
{
	BOOL		bRet = TRUE;
	unsigned int	uBytesToRead, i;
	char *pOrigBuffer = pOutBuffer;
	unsigned char ecc_calc[24];
	unsigned char ecc_code[24];
	int eccsteps;
	int datidx;
	int eccidx;
	/* WARNING : During a read procedure you can't call the ReadStatus flash cmd */
	/* The ReadStatus fill the read register with 0xC0 and then corrupt the read.*/

	/* Enable the chip */
	NAND_ENABLE_CE();

	/* Write specific command, Read from start */
        WRITE_NAND_COMMAND(CMD_READ_1);

	/* Push offset address */
	switch(fZone)
	{
		case ZONE_DATA:
                        /* Write specific command, Read from start */
			uBytesToRead = NAND_PAGE_SIZE; //pNandInfo->uDataNbBytes;
			WRITE_NAND_ADDRESS(0x00);
			WRITE_NAND_ADDRESS(0x00);
			break;
		case ZONE_INFO:
                        /* Write specific command, Read from start */
			uBytesToRead = NAND_OOB_SIZE; //pNandInfo->uSpareNbBytes;
			pOutBuffer += NAND_PAGE_SIZE; //pNandInfo->uDataNbBytes;
                        if (nand_16bit (pNandInfo))
			{	/* 16 bits */
				WRITE_NAND_ADDRESS(((NAND_PAGE_SIZE/2) >>  0) & 0xFF); /* Div 2 is because we address in word and not
				in byte */
				WRITE_NAND_ADDRESS(((NAND_PAGE_SIZE/2) >>  8) & 0xFF);
			} else { /* 8 bits */
				WRITE_NAND_ADDRESS((NAND_PAGE_SIZE >>  0) & 0xFF);
				WRITE_NAND_ADDRESS((NAND_PAGE_SIZE >>  8) & 0xFF);
			}
			break;
		case ZONE_DATA | ZONE_INFO:
			uBytesToRead = NAND_PAGE_SIZE + NAND_OOB_SIZE; //pNandInfo->uSectorNbBytes;
			WRITE_NAND_ADDRESS(0x00);
			WRITE_NAND_ADDRESS(0x00);
			break;
		default:
			bRet = FALSE;
			goto exit;
	}

	/* Push sector address */
	uSectorAddr >>= 11; //pNandInfo->uOffset;

	WRITE_NAND_ADDRESS((uSectorAddr >>  0) & 0xFF);
	WRITE_NAND_ADDRESS((uSectorAddr >>  8) & 0xFF);
	WRITE_NAND_ADDRESS((uSectorAddr >> 16) & 0xFF);

	WRITE_NAND_COMMAND(CMD_READ_2);

	/* Wait for flash to be ready (can't poll on status, read upper WARNING) */
	NAND_WAIT_READY();
	NAND_WAIT_READY();	/* Need to be done twice, READY detected too early the first time? */

        for(i=0; i<uBytesToRead; i+=NAND_OOB_SIZE)
            at91_read_data (at91, AT91C_SMARTMEDIA_BASE, (unsigned char *)&pOutBuffer[i], NAND_OOB_SIZE, 0);

	if (fZone == (ZONE_DATA|ZONE_INFO))
	{
		/* Calculate the ECC for the buffer */
		eccsteps = NAND_PAGE_SIZE / 256; // pNandInfo->uDataNbBytes/256;
		for (i=eccsteps, datidx=0, eccidx=0; i; i--, datidx+=256, eccidx+=3)
			nand_calculate_ecc((unsigned char *)&pOrigBuffer[datidx], &ecc_calc[eccidx]);

		/* Pick the ECC bytes out of the oob data */

		for (i=0; i < eccsteps*3; i++)
			ecc_code[i] = pOrigBuffer[((NAND_PAGE_SIZE + NAND_OOB_SIZE)-eccsteps*3)+i];

		/* Correct the ECC if necessary */
		for (i=eccsteps, datidx=0, eccidx=0; i; i--, datidx+=256, eccidx+=3)
			if (nand_correct_data((unsigned char *)&pOrigBuffer[datidx], &ecc_code[eccidx], &ecc_calc[eccidx]) == -1) {
                            fprintf (stderr, "NAND error\n");
                            bRet = -1;
                            goto exit;
                        }
	}

exit:
	/* Disable the chip */
	NAND_DISABLE_CE();

	return bRet;
}

int nand_read (at91_t *at91, unsigned int from, char *data, int length)
{
    int i;
    for (i = 0; i < length; i+=NAND_PAGE_SIZE) {
        progress ("NAND Read", i, length);
        if (nand_read_page (at91, from + i, &data[i], ZONE_DATA) < 0)
            return -1;
    }
    progress ("NAND Read", length, length);
    return 0;
}

int nand_read_file (at91_t *at91, unsigned int from, const char *filename, int length)
{
    char buffer[NAND_PAGE_SIZE];
    FILE *fp;
    int i;

    if ((fp = fopen (filename, "wb")) == NULL) {
        fprintf (stderr, "Unable to open '%s': %s [%d]\n", filename, strerror (errno), errno);
        return -1;
    }

    for (i = 0; i < length; i+=NAND_PAGE_SIZE) {
        progress ("NAND Read File", i, length);
        if (nand_read_page (at91, from + i, buffer, ZONE_DATA) < 0) {
            fclose (fp);
            return -1;
        }
        fwrite (buffer, 1, sizeof (buffer), fp);
    }
    progress ("NAND Read File", length, length);
    fclose (fp);
    return 0;
}

int nand_block_bad (at91_t *at91, unsigned int addr)
{
    unsigned char page[NAND_PAGE_SIZE + NAND_OOB_SIZE];
    unsigned char bad;
    int block = ADDR2BLOCK(addr);
    if (block > MAX_BLOCKS) {
        fprintf(stderr, "Address too large: 0x%x\n", addr);
        return -1;
    }
    if (bbt[block] == -1) {
        if (nand_read_page (at91, addr, (char *)page, ZONE_INFO) < 0)
            return -1;
        bad = page[NAND_PAGE_SIZE + 0];
        bbt[block] = (bad != 0xff);
    }
    return bbt[block];
}

int nand_erase_block (at91_t *at91, unsigned int addr)
{
    int e = nand_block_bad(at91, addr);
    if (e < 0)
        return -1;
    if (e) {
        printf("\nSkipping bad block @0x%8.8x\n", addr);
        return 0;
    }
    NAND_ENABLE_CE();
    /* Push Erase_1 command */
    WRITE_NAND_COMMAND(CMD_ERASE_1);

    addr >>= 11;

    /* Push block address in three cycles */
    WRITE_NAND_ADDRESS ((addr >> 0) & 0xff);
    WRITE_NAND_ADDRESS ((addr >> 8) & 0xff);
    WRITE_NAND_ADDRESS ((addr >> 16) & 0xff);

    WRITE_NAND_COMMAND (CMD_ERASE_2);

    NAND_WAIT_READY ();
    NAND_WAIT_READY ();

    /* Check status bit for error notification */
    WRITE_NAND_COMMAND(CMD_STATUS);
    NAND_WAIT_READY();
    if (READ_NAND() & STATUS_ERROR)
    {
        fprintf (stderr, "NAND Erase failure @ 0x%8.8x\n", addr);
        /* Error during block erasing */
        NAND_DISABLE_CE();
        return -1;
    }

    NAND_DISABLE_CE();

    return 0;
}

int nand_erase (at91_t *at91, unsigned int addr, unsigned int length)
{
    int i;
    for (i = 0; i < length; i+= NAND_BLOCK_SIZE) {
        progress ("NAND Erase", i, length);
        nand_erase_block (at91, addr + i);
    }
    progress ("NAND Erase", length, length);
    return 0;
}

static int nand_write_page (at91_t *at91, unsigned int addr, const char *data)
{
    int eccsteps, i, datidx, eccidx;
    unsigned char oob[NAND_OOB_SIZE];

    memset (oob, 0xff, sizeof (oob));

    eccsteps = NAND_PAGE_SIZE / 256; // pNandInfo->uDataNbBytes/256;
    for (i=eccsteps, datidx=0, eccidx=40; i; i--, datidx+=256, eccidx+=3)
        nand_calculate_ecc((unsigned char *)&data[datidx], &oob[eccidx]);

    NAND_ENABLE_CE();
    WRITE_NAND_COMMAND (CMD_WRITE_1);
    /* Push block address in three cycles */
    addr >>= 11;

    WRITE_NAND_ADDRESS (0x0);
    WRITE_NAND_ADDRESS (0x0);
    WRITE_NAND_ADDRESS ((addr >> 0) & 0xff);
    WRITE_NAND_ADDRESS ((addr >> 8) & 0xff);
    WRITE_NAND_ADDRESS ((addr >> 16) & 0xff);

    WRITE_NAND_BLOCK ((unsigned char *)data, NAND_PAGE_SIZE);
    WRITE_NAND_BLOCK (oob, sizeof (oob));
#if 0
    for (i = 0; i < NAND_PAGE_SIZE; i++)
        WRITE_NAND (data[i]);
    for (i = 0; i < sizeof (oob); i++)
        WRITE_NAND (oob[i]);
#endif

    WRITE_NAND_COMMAND (CMD_WRITE_2);

    NAND_WAIT_READY ();
    NAND_WAIT_READY ();

    /* Check status bit for error notification */
    WRITE_NAND_COMMAND(CMD_STATUS);
    NAND_WAIT_READY();
    if (READ_NAND() & STATUS_ERROR) {
        /* Error during block erasing */
        NAND_DISABLE_CE();
        return -1;
    }

    NAND_DISABLE_CE();

    return 0;
}

int nand_write (at91_t *at91, unsigned int addr, const char *data, int length)
{
    char page[NAND_PAGE_SIZE];
    int i, written = 0;
    int block = -1;

    if (addr & (NAND_PAGE_SIZE - 1)) {
        fprintf (stderr, "NAND write failure: Address 0x%8.8x not on a page boundary\n", addr);
        return -1;
    }

    for (i = 0; written < length; ) {
        int remaining = length - written;
        if (ADDR2BLOCK(addr + i) != block) {
            block = ADDR2BLOCK(addr+i);
            if (nand_block_bad(at91, addr + i)) {
                printf("Skipping write @ 0x%8.8x - bad block                 \n", 
                        addr + i);
                i += NAND_BLOCK_SIZE;
                continue;
            }
        }
        if (remaining < NAND_PAGE_SIZE) {
            memcpy (page, &data[written], remaining);
            memset (&page[remaining], 0xff, NAND_PAGE_SIZE - remaining);
            nand_write_page (at91, addr + i, page);
        } else
            nand_write_page (at91, addr + i, &data[written]);
        written += NAND_PAGE_SIZE;
        i += NAND_PAGE_SIZE;
    }

    return i;
}

int nand_write_file (at91_t *at91, unsigned int addr, const char *filename)
{
    FILE *fp;
    char buffer[NAND_PAGE_SIZE];
    int len;
    int total_length;
    int pos = 0, written = 0;
    struct stat buf;

    if (stat (filename, &buf) < 0) {
        fprintf (stderr, "NAND write failure: Unable to stat '%s': %s\n", filename, strerror (errno));
        return -1;
    }

    total_length = buf.st_size;

    if ((fp = fopen (filename, "rb")) == NULL) {
        fprintf (stderr, "NAND write failure: Unable to open '%s': %s\n", filename, strerror (errno));
        return -1;
    }

    while (!feof (fp)) {
        progress ("NAND Write File", written, total_length);
        len = fread(buffer, 1, sizeof (buffer), fp);
        if (len < 0) {
            fprintf (stderr, "Failed to read from '%s': %s\n", filename, strerror (errno));
            fclose (fp);
            return -1;
        }
        if (len == 0)
            break;
        pos += nand_write(at91, addr + pos, buffer, len);
        written += len;
    }
    progress ("NAND Write File", written, total_length);

    fclose (fp);

    return 0;
}

static int nand_write_raw_page (at91_t *at91, unsigned int addr, const char *data)
{
    int eccsteps, i, datidx, eccidx;
    unsigned char oob[64];
    
    memcpy (oob, &data[NAND_PAGE_SIZE], sizeof (oob));

    //printf ("Getting ecc...\n");
    eccsteps = NAND_PAGE_SIZE / 256; // pNandInfo->uDataNbBytes/256;
    for (i=eccsteps, datidx=0, eccidx=40; i; i--, datidx+=256, eccidx+=3)
        nand_calculate_ecc((unsigned char *)&data[datidx], &oob[eccidx]);

    NAND_ENABLE_CE();
    /* Push block address in three cycles */
    addr >>= 11;

    WRITE_NAND_COMMAND (CMD_WRITE_1);
    WRITE_NAND_ADDRESS (0x0);
    WRITE_NAND_ADDRESS (0x0);
    WRITE_NAND_ADDRESS ((addr >> 0) & 0xff);
    WRITE_NAND_ADDRESS ((addr >> 8) & 0xff);
    WRITE_NAND_ADDRESS ((addr >> 16) & 0xff);

    WRITE_NAND_BLOCK ((unsigned char *)data, NAND_PAGE_SIZE);
    WRITE_NAND_BLOCK (oob, sizeof (oob));

    WRITE_NAND_COMMAND (CMD_WRITE_2);
    NAND_WAIT_READY ();

    NAND_DISABLE_CE();

    return 0;
}

int nand_write_raw (at91_t *at91, unsigned int addr, const char *data, int length)
{
    int i;
    int npages;

    if (length % (NAND_PAGE_SIZE + NAND_OOB_SIZE)) {
        fprintf (stderr, "NAND Write Raw: Data not a multiple of %d\n", NAND_PAGE_SIZE + NAND_OOB_SIZE);
        return -1;
    }

    npages = length / (NAND_PAGE_SIZE + NAND_OOB_SIZE);

    for (i = 0; i < npages; i++) {
        progress ("NAND Raw Write", i * (NAND_PAGE_SIZE + NAND_OOB_SIZE), length);
        if (nand_write_raw_page (at91, addr + (i * NAND_PAGE_SIZE), &data[i * (NAND_PAGE_SIZE + NAND_OOB_SIZE)]) < 0)
            return -1;
    }
    progress ("NAND Raw Write", length, length);

    return 0;

}

int nand_write_raw_file (at91_t *at91, unsigned int addr, const char *filename)
{
    FILE *fp;
    char buffer[NAND_PAGE_SIZE + NAND_OOB_SIZE];
    int length;
    int pos = 0;
    struct stat buf;
    int pages = 0;
    int block = -1;

    if (stat (filename, &buf) < 0) {
        fprintf (stderr, "Unable to stat '%s': %s\n", filename, strerror (errno));
        return -1;
    }

    length = buf.st_size;

    if (length % (NAND_PAGE_SIZE + NAND_OOB_SIZE)){
        fprintf (stderr, "NAND Write File: File size not a multiple of %d\n", 
                NAND_PAGE_SIZE + NAND_OOB_SIZE);
        return -1;
    }

    if ((fp = fopen (filename, "rb")) == NULL) {
        fprintf (stderr, "Unable to open '%s': %s\n", filename, strerror (errno));
        return -1;
    }
    
    while (!feof (fp)) {
        int l = fread (buffer, 1, sizeof (buffer), fp);
        if (l < 0) {
            fprintf (stderr, "Read error: %s\n", strerror (errno));
            fclose (fp);
            return -1;
        }
        if (l == 0)
            break;
        if (l != sizeof (buffer)) {
            fprintf (stderr, "Unable to read enough bytes from '%s'\n", filename);
            fclose (fp);
            return -1;
        }
        if (ADDR2BLOCK(addr + pages) != block) {
            do {
                int e;
                e = nand_block_bad(at91, addr + pages);
                if (e < 0)
                    return -1;
                if (e == 0)
                    break;
                pages += PAGES_PER_BLOCK;
            } while (1);
            block = ADDR2BLOCK(addr + pages);
        }
        nand_write_raw_page (at91, addr + pages * NAND_PAGE_SIZE, buffer);
        progress ("NAND Write Raw File", pos, length);
        pos += l;
        pages++;
    }
    progress ("NAND Write Raw File", pos, length);

    fclose (fp);

    return 0;
}

