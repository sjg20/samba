#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <usb.h>

#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include "at91sam.h"
#include "common.h"

#define MAX_BLOCK_SIZE 64
#define USB_TIMEOUT 1000

#define EP_IN 0x82
#define EP_OUT 0x01 // from lsusb -v

#if 0
#define dbg(a,b...) printf ("DBG: " a "\n" , ##b)
#else
#define dbg(a,b...) do {} while (0)
#endif

struct at91_t {
    usb_dev_handle *usb;
    int serial;
};

/** FIXME: Should take the usb/serial bits out into a separate file, 
 * so we can port to Win32 a bit easier in the future
 */

static struct usb_device *find_usb_device(int vendor_id, int product_id)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_get_busses(); bus; bus = bus->next)
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

at91_t *at91_open_serial(const char *device)
{
    at91_t *at91 = NULL;
    struct termios options;

    at91 = malloc (sizeof (at91));
    if (!at91) {
        fprintf (stderr, "Out of memory\n");
        goto err;
    }
    at91->serial = open (device, O_RDWR | O_NOCTTY);
    if (at91->serial < 0) {
        fprintf (stderr, "Unable to open '%s': %s\n", device, strerror (errno));
        goto err;
    }
    signal (SIGIO, SIG_IGN);

    fcntl (at91->serial, F_SETFL, 0);
    if (tcgetattr(at91->serial, &options) < 0) {
        fprintf (stderr, "Unable to get TC info\n");
        goto err;
    }

    cfsetospeed (&options, B115200);
    cfsetispeed (&options, B115200);
    options.c_cflag &= ~(HUPCL | PARENB | CSTOPB | CSIZE | CRTSCTS);
    options.c_cflag |= CREAD | CLOCAL | CS8;

    if (tcsetattr(at91->serial, TCSANOW, &options) < 0) {
        fprintf (stderr, "Unable to set TC info on '%s'\n", device);
        goto err;
    }

    printf ("Connected to '%s'\n", device);

    return at91;
err:
    if (at91) {
        if (at91->serial)
            close (at91->serial);
        free (at91);
    }
    return NULL;
}

at91_t *at91_open_usb(int vendor_id, int product_id)
{
    struct usb_device *dev;
    at91_t *at91 = NULL;
    static int inited = 0;

    if (!inited) {
        usb_init ();
        usb_find_busses();
        usb_find_devices();
        inited = 1;
    }

    dev = find_usb_device (vendor_id, product_id);

    if (!dev) {
        fprintf (stderr, "Unable to find Atmel AT91-SAM device\n");
        goto err;
    }
    at91 = malloc (sizeof (at91_t));
    if (!at91) {
        fprintf (stderr, "Out of memory\n");
        goto err;
    }
    at91->serial = -1;

    at91->usb = usb_open (dev);

    if (!at91->usb) {
        fprintf (stderr, "Unable to open Atmel AT91-SAM device\n");
        fprintf (stderr, "\tdo you have sufficient permissions?\n");
        goto err;
    }

    //if (usb_set_configuration(at91, 0) < 0) {
        //fprintf(stderr, "Error: setting config 0 failed\n");
        //goto err;
    //}

    if(usb_claim_interface(at91->usb, 1) < 0) {
        perror("error: claiming interface 1 failed\n");
        goto err;
    }

    printf ("Connected to USB\n");

    return at91;
err:
    if (at91) {
        if (at91->usb)
            usb_close (at91->usb);
        free (at91);
    }
    return NULL;
}

void at91_close (at91_t *at91)
{
    if (!at91)
        return;
    if (at91->usb) {
        usb_release_interface(at91->usb, 1);
        usb_close(at91->usb);
        at91->usb = NULL;
    }
    if (at91->serial >= 0) {
        close (at91->serial);
        at91->serial = -1;
    }
    free (at91);
}

static int at91_write (at91_t *at91, const unsigned char *buffer, int len)
{
    int write_len;

    if (!at91) {
        fprintf (stderr, "AT91 not connected\n");
        return -1;
    }

    dbg ("Sending %d bytes '%s'", len, buffer);

    if (at91->usb) {
        assert (len <= MAX_BLOCK_SIZE);

        write_len = usb_bulk_write(at91->usb, EP_OUT, (char *)buffer, len, USB_TIMEOUT);
        if (write_len < 0) {
            fprintf (stderr, "USB Write failure: %s\n", strerror (errno));
            return -1;
        }
        if (write_len != len) {
            fprintf (stderr, "usb bulk write was odd: %d != %d\n", 
                    write_len, len);
            return -1;
        }
    } else if (at91->serial >= 0) {
        write_len = write(at91->serial, buffer, len);
        if (write_len < 0) {
            fprintf (stderr, "Serial write failure: %s\n", strerror (errno));
            return -1;
        }
        if (write_len != len) {
            fprintf (stderr, "Insufficient serial data sent (%d < %d)\n", 
                    write_len, len);
            return -1;
        }
    } else {
        fprintf (stderr, "Not connected via either USB or serial\n");
        return -1;
    }
    return 0;
}

static int at91_read (at91_t *at91, unsigned char *buffer, int len)
{
    int read_len;

    if (!at91) {
        fprintf (stderr, "AT91 not connected\n");
        return -1;
    }

    if (at91->usb) {
        assert (len <= MAX_BLOCK_SIZE);
        read_len = usb_bulk_read(at91->usb, EP_IN, (char *)buffer, len, USB_TIMEOUT);
        if (read_len < 0) {
            fprintf (stderr, "usb bulk read failure: %d [%s]\n", errno, strerror (errno));
            return -1;
        }
    } else if (at91->serial >= 0) {
        fd_set fds;
        struct timeval timeout;
        int retval;

        FD_ZERO (&fds);
        FD_SET (at91->serial, &fds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        retval = select (at91->serial + 1, &fds, NULL, NULL, &timeout);
        if (retval < 0) {
            fprintf (stderr, "serial select failure: %d [%s]\n", errno, strerror (errno));
            return -1;
        } else if (retval == 0) {
            fprintf (stderr, "Timeout waiting for AT91 data\n");
            return 0;
        }
        read_len = read(at91->serial, buffer, len);
        if (read_len < 0) {
            fprintf (stderr, "serial read failure: %d [%s]\n", errno, strerror (errno));
            return -1;
        }
    } else {
        fprintf (stderr, "AT91 not connected via either usb or serial\n");
        return -1;
    }
#if 0
    {
        int i;
        printf ("Read %d bytes\n", read_len);
        for (i = 0; i < read_len; i++)
            printf ("%d: 0x%x '%c'\n", i, buffer[i], buffer[i]);
    }
#endif
    return read_len;
}

static int at91_read_len (at91_t *at91, unsigned char *buffer, int len)
{
    int total_len = 0;

    do {
        int this_len = at91_read (at91, &buffer[total_len], len - total_len);
        if (this_len < 0)
            return -1;
        total_len += this_len;
    } while (total_len < len);
    assert (total_len == len);
    return total_len;
}

static int at91_wait_for_prompt (at91_t *at91, char *result, int max_len, unsigned char prompt)
{
    int len;
    unsigned char tmp[MAX_BLOCK_SIZE];
    char *pos;
    int count = 0;
    int i; 
#if 1
    char tmp_response[100];

    if (!result) {
        result = tmp_response;
        max_len = sizeof (tmp_response);
    }
#endif

    pos = result;

    if (!at91) {
        fprintf (stderr, "AT91 not connected\n"); 
        return -1;
    }
   
    //printf ("Waiting for '%c' [%d] prompt\n", prompt, prompt);
    while (count < 5) {
        len = at91_read (at91, tmp, sizeof (tmp));
        dbg ("Got %d prompt chars", len);
        //for (i = 0; i < len; i++)
            //printf ("%d: %d [%c, 0x%x]\n", i, tmp[i], tmp[i], tmp[i]);
        for (i = 0; i < len; i++) {
            if (tmp[i] == prompt) {
                if (pos)
                    *pos = '\0';
                dbg ("Got response: %d '%s'", pos - result, result);
                return pos - result;
            }
            else if (pos != result || strchr ("\r\t\n ", tmp[i]) == NULL) // not white space, or not first char
                if (pos && pos - result < max_len)
                    *pos++ = tmp[i];
        }
        if (len <= 0) // if we actually get data, try to avoid timing out
            count++;
        //printf ("Waiting for prompt %d\n", count);
    }
    
    fprintf (stderr, "Error: Timeout waiting for AT91 prompt %c\n", prompt);
    return -1;
}

int at91_init (at91_t *at91)
{
    unsigned char init[] = {0x80, 0x80, 0x80, 0x80, 0x80, '#', '#', '#', '#'};
    int i;

    if (at91->usb)
        return 0;
    else if (at91->serial >= 0) {
        for (i = 0; i < sizeof (init); i++) {
            if (at91_write(at91, &init[i], 1) < 0) {
                fprintf(stderr, "Error: init data write failure\n");
                return -1;
            }
            usleep (100 * 1000); // send the characters out slowly, to try and catch the unit
        }

        if (at91_wait_for_prompt (at91, NULL, 0, '>') < 0)
            return -1;
    } else {
        fprintf (stderr, "AT91 not connected\n");
        return -1;
    }
    

    return 0;
}

static int at91_command (at91_t *at91, char *string,...)
{
    va_list ap;
    char buffer[200];
    int len;

    va_start (ap,string);
    len = vsnprintf (buffer, sizeof (buffer) - 1, string, ap);
    va_end (ap);

    if (len <= 0) {
        fprintf (stderr, "Attempt to send nul at91 command\n");
        return -1;
    }
    //buffer[len++] = '\r';
    //buffer[len++] = '\n';
    //buffer[len++] = '\0';
    if (at91_write(at91, (unsigned char *)buffer, len) < 0) {
        fprintf (stderr, "Unable to write '%s' AT91 command\n", buffer);
        return -1;
    }
    //printf ("Sent command: %s\n", buffer);
    return 0;
}

int at91_read_byte(at91_t *at91, unsigned int addr)
{
    char buffer[100];

    dbg ("read byte @0x%x", addr);

    if (at91_command (at91, "o%X,#", addr) < 0)
        return -1;
    if (at91_wait_for_prompt (at91, buffer, sizeof (buffer), '>') < 0)
        return -1;
    return strtoul(buffer, NULL, 16);
}

int at91_read_half_word (at91_t *at91, unsigned int addr)
{
    char buffer[100];

    dbg ("read half word @0x%x", addr);

    if (at91_command (at91, "h%X,#", addr) < 0)
        return -1;
    if (at91_wait_for_prompt (at91, buffer, sizeof (buffer), '>') < 0)
        return -1;
    return strtoul(buffer, NULL, 16);
}

int at91_read_word (at91_t *at91, unsigned int addr)
{
    char buffer[100];

    dbg ("read word @0x%x", addr);

    if (at91_command (at91, "w%X,#", addr) < 0)
        return -1;
    if (at91_wait_for_prompt (at91, buffer, sizeof (buffer), '>') < 0)
        return -1;
    return strtoul(buffer, NULL, 16);
}

int at91_write_byte (at91_t *at91, unsigned int addr, unsigned char value)
{
    dbg ("write byte @0x%x: 0x%x", addr, value);

    if (at91_command (at91, "O%X,%2.2X#", addr, value) < 0)
        return -1;
    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_write_half_word (at91_t *at91, unsigned int addr, unsigned int value)
{
    dbg ("write half word @0x%x: 0x%x", addr, value);

    if (at91_command (at91, "H%X,%4.4X#", addr, value) < 0)
        return -1;
    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_write_word (at91_t *at91, unsigned int addr, unsigned int value)
{
    dbg ("write word @0x%x: 0x%x", addr, value);

    if (at91_command (at91, "W%X,%X#", addr, value) < 0)
        return -1;
    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_version (at91_t *at91, char *result, int max_len)
{
    char *pos;
    dbg ("version");

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

static int at91_wait_for_ack (at91_t *at91)
{
    unsigned char tmp;
    int len;

    dbg ("Waiting for ACK");
    len = at91_read (at91, &tmp, 1);
    if (len < 0)
        return -1;
    if (len == 0) {
        fprintf (stderr, "ACK timeout\n");
        return -1;
    }
    if (tmp == 0x18) {
        fprintf (stderr, "Transfer cancelled by other end (CRC Error)\n");
        return -1;
    }
    if (tmp != 0x6) {
        fprintf (stderr, "Didn't get an ACK, got 0x%x\n", tmp);
        return -1;
    }
    return 0;
}

static uint16_t crc16(unsigned char *s, int len)
{
    uint32_t crc = 0;
    int i;

    while (len--) {
        crc = crc ^ ((*s++) << 8);
        for (i = 0; i < 8; i++)
            if (crc & 0x8000)
                crc = ((crc << 1) ^ 0x1021);
            else
                crc = crc << 1;
    }
    return crc & 0xffff;
}

static int at91_write_data_xmodem (at91_t *at91, unsigned int addr, 
        const unsigned char *data, int len)
{
    int block = 1;
    int pos = 0;
    unsigned char send_data[128 + 5];
    uint16_t crc;

    if (len == 0)
        return 0;
    if (at91_command (at91, "S%X,#", addr) < 0)
        return -1;

    if (at91_wait_for_prompt (at91, NULL, 0, 'C') < 0)
        return -1;

    while (pos < len) {
        int this_len = min(128, len - pos);
        memcpy (&send_data[3], &data[pos], this_len);
        if (this_len < 128)
            memset (&send_data[3 + this_len], 0xff, 128 - this_len);
        send_data[0] = 0x01;
        send_data[1] = block;
        send_data[2] = 255 - block;
        crc = crc16(&send_data[3], 128);
        send_data[128 + 3] = crc >> 8;
        send_data[128 + 4] = crc;

        dbg ("Sending block '%d' [%d]", block, this_len);
        if (at91_write (at91, send_data, sizeof (send_data)) < 0)
            return -1;
    
        if (at91_wait_for_ack (at91) < 0)
            return -1;
        block = (block + 1) % 256;
        pos += 128;
    }

   
    send_data[0] = 0x04;
    if (at91_write (at91, send_data, 1) < 0)
        return -1;
    if (at91_wait_for_ack (at91) < 0)
        return -1;

    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

static int  at91_read_data_xmodem (at91_t *at91, unsigned int addr, 
        unsigned char *data, int len)
{
    int block = 1;
    int pos = 0;
    unsigned char tmp[2];
    uint16_t crc;

    if (at91_command (at91, "R%X,%X#", addr, len) < 0)
        return -1;
    if (at91_command (at91, "C") < 0)
        return -1;

    if (len % 128)
        fprintf (stderr, "Warning: Attempting to read a non 128 aligned length (%d bytes @ 0x%x)\n", len, addr);

    while (pos < len) {
        dbg ("xmodem reading block %d", block);
        do {
            if (at91_read_len (at91, tmp, 1) < 0)
                return -1;
            if (tmp[0] != 0x01) {
                fprintf (stderr, "Unexpected SOF: 0x%x\n", tmp[0]);
                //return -1;
            }
        } while (tmp[0] != 0x01);
        if (at91_read_len (at91, tmp, 2) < 0)
            return -1;
        if (tmp[0] != block || tmp[1] != 255 - block) {
            fprintf (stderr, "Unexpected block received: %d/%d vs %d/%d\n", 
                    tmp[0], tmp[1], block, 255 - block);
            return -1;
        }
        if (at91_read_len (at91, &data[pos], 128) < 0)
            return -1;
        if (at91_read_len (at91, tmp, 2) < 0)
            return -1;
        // FIXME: Check CRC?
        block = (block + 1) % 256;
        pos += 128;


        tmp[0] = 0x06;
        if (at91_write(at91, tmp, 1) < 0)
            return -1;
    }

    return at91_wait_for_prompt (at91, NULL, 0, '>');
}

int at91_read_data (at91_t *at91, unsigned int address, unsigned char *data, int length)
{
    unsigned char discard[2];
    int i;

    if (length == 0)
        return 0;

    dbg ("read data @0x%x: 0x%x", address, length);

    if (at91->usb) {
        for (i = 0; i < length; i += MAX_BLOCK_SIZE) {
            int this_len = min(MAX_BLOCK_SIZE, length - i);
            if (at91_command (at91, "R%X,%X#", address + i, this_len) < 0)
                return -1;
            if (at91_read (at91, discard, 2) < 0) // drop carriage return
                return -1;
            if (at91_read (at91, &data[i], this_len) < 0)
                return -1;
            if (at91_wait_for_prompt (at91, NULL, 0, '>') < 0)
                return -1;
        }
        return 0;
    } else if (at91->serial >= 0)
        return at91_read_data_xmodem (at91, address, data, length);

    fprintf (stderr, "AT91 not connected\n");
    return -1;
}

int at91_read_file (at91_t *at91, unsigned int address, const char *filename, int length)
{
    FILE *fp;
    unsigned char buffer[8192];
    unsigned int pos = 0;

    dbg ("read file @0x%x: 0x%x", address, length);

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

    dbg ("verify file @0x%x: %s", address, filename);

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
    dbg ("write data @0x%x: 0x%x", address, length);

    if (length == 0)
        return 0;

    if (at91->usb) {
        for (i = 0; i < length; i += MAX_BLOCK_SIZE) {
            int this_len = min(MAX_BLOCK_SIZE, length - i);
            if (at91_command (at91, "S%X,%X#", address + i, this_len) < 0)
                return -1;
            if (at91_write (at91, &data[i], this_len) < 0)
                return -1;
            if (at91_wait_for_prompt (at91, NULL, 0, '>') < 0)
                return -1;
        }
        return 0;
    } else if (at91->serial >= 0) {
        return at91_write_data_xmodem (at91, address, data, length);
    }
    return -1;
}

int at91_write_file (at91_t *at91, unsigned int address, const char *filename)
{
    FILE *fp;
    unsigned char buffer[8192];
    unsigned int pos = address;
    struct stat buf;

    dbg ("write file @0x%x: %s", address, filename);

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
    unsigned char discard;
    dbg ("go @0x%x", address);

    if (at91_command (at91, "G%X#", address) < 0)
        return -1;
    if (at91->usb) // have to get response when using USB, or it doesn't complete
        if (at91_read (at91, &discard, 1) < 0) // drop carriage return
            return -1;
    return 0;
}


