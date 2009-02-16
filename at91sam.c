#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <usb.h>

#include "at91sam.h"
#include "common.h"

#define MAX_BLOCK_SIZE 64


#define EP_IN 0x82
#define EP_OUT 0x01 // from lsusb -v

static int at91_wait_for_prompt (at91_t *at91, char *result, int max_len, char prompt);
static int at91_write (at91_t *at91, char *buffer, int len);

#if 0
static unsigned int crc16 (unsigned char *data, int length) // '123456789' == 31c3
{
    int crc = 0;
    int i;

    while (length--) {
        crc = crc ^ (*data++ << 8);
        for (i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = ((crc << 1) ^ 0x1021);
            else
                crc <<= 1;
        }
    }
    return (crc & 0xffff);
}
#endif

static struct usb_device *find_usb_device(int vendor_id, int product_id)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_busses; bus; bus = bus->next)
        for (dev = bus->devices; dev; dev = dev->next)
            if (dev->descriptor.idVendor == vendor_id
                    && dev->descriptor.idProduct == product_id)
            {
                char fname[30];
                sprintf (fname, "/proc/bus/usb/%s/%s", bus->dirname, 
                        dev->filename);
                if (access (fname, R_OK | W_OK) < 0) {
                    sprintf (fname, "/dev/bus/usb/%s/%s", bus->dirname, 
                            dev->filename);
                    if (access (fname, R_OK | W_OK) < 0) {
                        char command[100];
                        fprintf (stderr, "Warning: insufficient permissions to USB device files\n");
                        sprintf (command, "sudo chmod a+rw %s", fname);
                        printf ("command: %s\n", command);
                        system (command);
                    }
                }
                return dev;
            }
    return NULL;
}

at91_t *at91_open(int vendor_id, int product_id)
{
    struct usb_device *dev;
    usb_dev_handle *at91 = NULL;
    char init[3] = {0x80, 0x80, '#'};

    /** FIXME - may be doing these twice? */
    usb_init ();
    usb_find_busses();
    usb_find_devices();

    dev = find_usb_device (vendor_id, product_id);

    if (!dev) {
        fprintf (stderr, "Unable to find Atmel AT91-SAM device\n");
        goto err;
    }

    at91 = usb_open (dev);

    if (!at91) {
        fprintf (stderr, "Unable to open Atmel AT91-SAM device\n");
        fprintf (stderr, "\tdo you have sufficient permissions?\n");
        goto err;
    }

    //if (usb_set_configuration(at91, 0) < 0) {
        //fprintf(stderr, "Error: setting config 0 failed\n");
        //goto err;
    //}

    if(usb_claim_interface(at91, 1) < 0) {
        perror("error: claiming interface 1 failed\n");
        goto err;
    }

    if (at91_write(at91, init, 3) < 0) {
        fprintf(stderr, "Error: bulk write failed\n");
        goto err;
    }

    if (at91_wait_for_prompt (at91, NULL, 0, '>') < 0)
        goto err;

    return at91;
err:
    if (at91)
        usb_close (at91);
    return NULL;
}

void at91_close (at91_t *at91)
{
    usb_release_interface(at91, 0);
    usb_close(at91);
}

#if 0
static int at91_wait_for_ack (at91_t *at91)
{
    int len;
    char tmp[1000];
    int count = 0;
    int i;
    
    while (count < 10) {
        len=usb_bulk_read(at91, EP_IN, tmp, sizeof(tmp), 500);
        //printf ("Got %d chars\n", len);
        for (i = 0; i < len; i++) {
            //printf ("%d [%c]\n", tmp[i], tmp[i]);
            if (tmp[i] == 0x6) 
                return 0;
            else if (tmp[i] == 0x18) {
                fprintf (stderr, "Transfer cancelled by other end - CRC error?\n");
                return -1;
            } else {
                fprintf (stderr, "Unknown ack response - %d\n", tmp[i]);
                return -1;
            }
        }
        count++;
        //printf ("Waiting for ack %d\n", count);
    }
    
    fprintf (stderr, "Error: Timeout waiting for AT91 ack\n");
    return -1;
}
#endif

static int at91_wait_for_prompt (at91_t *at91, char *result, int max_len, char prompt)
{
    int len;
    char tmp[1000];
    char *pos = result;
    int count = 0;
    int i;
   
    //printf ("Waiting for '%c' [%d] prompt\n", prompt, prompt);
    while (count < 10) {
        len=usb_bulk_read(at91, EP_IN, tmp, sizeof(tmp), 500);
        //printf ("Got %d chars\n", len);
        for (i = 0; i < len; i++) {
            //printf ("%d [%c, 0x%x]\n", tmp[i], tmp[i], tmp[i]);
            if (tmp[i] == prompt) {
                if (pos)
                    *pos = '\0';
                return pos - result;
            }
            else if (pos != result || strchr ("\r\t\n ", tmp[i]) == NULL) // not white space
                if (pos && pos - result < max_len)
                    *pos++ = tmp[i];
        }
        count++;
        //printf ("Waiting for prompt %d\n", count);
    }
    
    fprintf (stderr, "Error: Timeout waiting for AT91 prompt %c\n", prompt);
    return -1;
}

static int at91_write (at91_t *at91, char *buffer, int len)
{
    int write_len;

    assert (len <= 64);
    write_len = usb_bulk_write(at91, EP_OUT, buffer, len, 500);
    if (write_len != len) {
        fprintf (stderr, "usb bulk write was odd: %d != %d\n", 
                write_len, len);
        return -1;
    }
    return 0;
}

static int at91_read (at91_t *at91, char *buffer, int len)
{
    int read_len;

    assert (len <= 64);

    read_len = usb_bulk_read(at91, EP_IN, buffer, len, 500);
    if (read_len < 0) {
        fprintf (stderr, "usb bulk read failure: %d [%s]\n", errno, strerror (errno));
        return -1;
    }
    return 0;
}

static int at91_command (usb_dev_handle *at91, char *string,...)
{
    va_list ap;
    char buffer[200];
    int len;

    va_start (ap,string);
    len = vsnprintf (buffer, sizeof (buffer) - 1, string, ap);
    va_end (ap);
    buffer[len] = '\0';

    if (len <= 0) {
        fprintf (stderr, "Attempt to send nul at91 command\n");
        return -1;
    }
    if (at91_write(at91, buffer, len) < 0) {
        fprintf (stderr, "Unable to write '%s' at91 command\n", buffer);
        return -1;
    }
    //printf ("Sent command: %s\n", buffer);
    return 0;
}

int at91_read_byte(at91_t *at91, unsigned int addr)
{
    char buffer[100];

    if (at91_command (at91, "o%X,#", addr) < 0)
        return -1;
    if (at91_wait_for_prompt (at91, buffer, sizeof (buffer), '>') < 0)
        return -1;
    return strtoul(buffer, NULL, 16);
}

int at91_read_half_word (at91_t *at91, unsigned int addr)
{
    char buffer[100];

    if (at91_command (at91, "h%X,#", addr) < 0)
        return -1;
    if (at91_wait_for_prompt (at91, buffer, sizeof (buffer), '>') < 0)
        return -1;
    return strtoul(buffer, NULL, 16);
}

int at91_read_word (at91_t *at91, unsigned int addr)
{
    char buffer[100];

    if (at91_command (at91, "w%X,#", addr) < 0)
        return -1;
    if (at91_wait_for_prompt (at91, buffer, sizeof (buffer), '>') < 0)
        return -1;
    return strtoul(buffer, NULL, 16);
}

int at91_write_byte (at91_t *at91, unsigned int addr, unsigned char value)
{
    if (at91_command (at91, "O%X,%X#", addr, value) < 0)
        return -1;
    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_write_half_word (at91_t *at91, unsigned int addr, unsigned int value)
{
    if (at91_command (at91, "H%X,%X#", addr, value) < 0)
        return -1;
    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_write_word (at91_t *at91, unsigned int addr, unsigned int value)
{
    if (at91_command (at91, "W%X,%X#", addr, value) < 0)
        return -1;
    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_version (at91_t *at91, char *result, int max_len)
{
    char *pos;
    if (at91_command (at91, "V#") < 0)
        return -1;
    if (at91_wait_for_prompt (at91, result, max_len, '>') < 0)
        return -1;

    // strip trailing carriage returns
    do {
        pos = strrchr (result, '\r');
        if (!pos)
            pos = strrchr (result, '\n');

        if (pos)
            *pos = '\0';
    } while(0);

    return 0;
}

int at91_read_data (at91_t *at91, unsigned int address, unsigned char *data, int length)
{
    char discard[2];
    int i;

    for (i = 0; i < length; i += MAX_BLOCK_SIZE) {
        int this_len = min(MAX_BLOCK_SIZE, length - i);
        if (at91_command (at91, "R%X,%X#", address + i, this_len) < 0)
            return -1;
        if (at91_read (at91, discard, 2) < 0) // drop carriage return
            return -1;
        if (at91_read (at91, (char *)&data[i], this_len) < 0)
            return -1;
        if (at91_wait_for_prompt (at91, NULL, 0, '>') < 0)
            return -1;
    }
    return 0;
}

int at91_read_file (at91_t *at91, unsigned int address, const char *filename, int length)
{
    FILE *fp;
    unsigned char buffer[8192];
    unsigned int pos = 0;

    if ((fp = fopen (filename, "wb")) == NULL) {
        fprintf (stderr, "Unable to open '%s': %s [%d]\n", filename, strerror (errno), errno);
        return -1;
    }

    while (pos < length) {
        progress ("Read File", pos, length);
        int len = min (sizeof (buffer), length - pos);
        if (at91_read_data (at91, address + pos, buffer, len) < 0) {
            fclose (fp);
            return -1;
        }
        if (fwrite (buffer, 1, len, fp) < 0) {
            fprintf (stderr, "Write failure on '%s': %s [%d]\n", filename, strerror (errno), errno);
            fclose (fp);
            return -1;
        }

        pos += len;
    }
    progress ("Read File", length, length);
    fclose (fp);
    return 0;
}

int at91_verify_file(at91_t *at91, unsigned int address, const char *filename)
{
    struct stat buf;
    unsigned char *data, *data2;
    FILE *fp;
    int val;

    if (stat (filename, &buf) < 0) {
        fprintf (stderr, "Unable to find '%s': %s\n", filename, strerror (errno));
        return -1;
    }

    data = malloc (buf.st_size);

    if (at91_read_data (at91, address, data, buf.st_size) < 0) {
        free (data);
        return -1;
    }

    if ((fp = fopen (filename, "rb")) == NULL) {
        fprintf (stderr, "Unable to open '%s': %s\n", filename, strerror (errno));
        free (data);
        return -1;
    }

    data2 = malloc (buf.st_size);

    fread (data2, 1, buf.st_size, fp);
    fclose (fp);

    val = memcmp (data, data2, buf.st_size);

    free (data);
    free (data2);

    return val == 0 ? 1 : 0;
}

int at91_write_data (at91_t *at91, unsigned int address, const unsigned char *data, int length)
{
    int i;
    if (length == 0)
        return 0;
    for (i = 0; i < length; i += MAX_BLOCK_SIZE) {
        int this_len = min(MAX_BLOCK_SIZE, length - i);
        if (at91_command (at91, "S%X,%X#", address + i, this_len) < 0)
            return -1;
        if (at91_write (at91, (char *)&data[i], this_len) < 0)
            return -1;
        if (at91_wait_for_prompt (at91, NULL, 0, '>') < 0)
            return -1;
    }
    return 0;
}

int at91_write_file (at91_t *at91, unsigned int address, const char *filename)
{
    FILE *fp;
    unsigned char buffer[8192];
    unsigned int pos = address;
    struct stat buf;

    if (stat (filename, &buf) < 0) {
        fprintf (stderr, "Unable to stat '%s': %s [%d]\n", filename, strerror (errno), errno);
        return -1;
    }


    if ((fp = fopen (filename, "rb")) == NULL) {
        fprintf (stderr, "Unable to open '%s': %s [%d]\n", filename, strerror (errno), errno);
        return -1;
    }

    while (!feof (fp)) {
        int len = fread (buffer, 1, sizeof (buffer), fp);
        progress ("Write File", pos - address, buf.st_size);

        if (len < 0) {
            fprintf (stderr, "Read failure on '%s': %s [%d]\n", filename, strerror (errno), errno);
            fclose (fp);
            return -1;
        }

        if (at91_write_data (at91, pos, buffer, len) < 0) {
            fclose (fp);
            return -1;
        }
        pos += len;
    }
    progress ("Write File", buf.st_size, buf.st_size);
    fclose (fp);
    return 0;
}

int at91_go (at91_t *at91, unsigned int address)
{
    char discard[2];
    if (at91_command (at91, "G%X#", address) < 0)
        return -1;
    if (at91_read (at91, discard, 2) < 0) // drop carriage return
        return -1;
    //return at91_wait_for_prompt (at91, NULL, 0, '>'); 
    return 0;
}

