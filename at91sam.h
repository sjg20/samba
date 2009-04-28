#ifndef AT91SAM_H
#define AT91SAM_H

#include "AT91SAM9260_inc.h"

typedef struct at91_t at91_t;

at91_t *at91_open_usb(int vendor_id, int product_id);
at91_t *at91_open_serial(const char *device);

void at91_close (at91_t *at91);
int at91_init (at91_t *at91);

int at91_version (at91_t *udev, char *result, int max_len);

int at91_read_byte(at91_t *at91, unsigned int addr);
int at91_read_half_word(at91_t *at91, unsigned int addr);
int at91_read_word(at91_t *at91, unsigned int addr);
int at91_read_data(at91_t *at91, unsigned int address, unsigned char *data, int length);
int at91_read_file(at91_t *at91, unsigned int address, const char *filename, int length);
int at91_verify_file(at91_t *at91, unsigned int address, const char *filename);

int at91_write_byte (at91_t *at91, unsigned int addr, unsigned char value);
int at91_write_half_word (at91_t *at91, unsigned int addr, unsigned int value);
int at91_write_word (at91_t *at91, unsigned int addr, unsigned int value);
int at91_write_data (at91_t *at91, unsigned int address, const unsigned char *data, int length);
int at91_write_file (at91_t *at91, unsigned int address, const char *filename);
int at91_go (at91_t *at9, unsigned int address);

#define writel(val,addr) at91_write_word(at91, addr, val)
#define readl(addr) at91_read_word(at91, addr)

#endif

