#ifndef NAND_H
#define NAND_H


#define SPARE_VALUE                     0xFF

#define ZONE_DATA                       0x01
#define ZONE_INFO                       0x02

/* Nand flash chip status codes */
#define STATUS_READY                (0x01<<6)   /* Status code for Ready */
#define STATUS_ERROR                (0x01<<0)   /* Status code for Error */

/* Nand flash commands */
#define CMD_READ_1                      0x00
#define CMD_READ_2                      0x30

#define CMD_READID                      0x90

#define CMD_WRITE_1                     0x80
#define CMD_WRITE_2                     0x10

#define CMD_ERASE_1                     0x60
#define CMD_ERASE_2                     0xD0

#define CMD_STATUS                      0x70
#define CMD_RESET                       0xFF


int nand_init (at91_t *at91);
int nand_read_id (at91_t *at91);
int nand_read_file (at91_t *at91, unsigned int from, const char *filename, int length);
int nand_read (at91_t *at91, unsigned int from, char *data, int length);

int nand_erase_block (at91_t *at91, unsigned int addr);
int nand_erase (at91_t *at91, unsigned int addr, unsigned int length);

int nand_calculate_ecc(const unsigned char *dat, unsigned char *ecc_code);
int nand_correct_data(unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc);

int nand_write (at91_t *at91, unsigned int addr, const char *data, int length);
int nand_write_file (at91_t *at91, unsigned int addr, const char *filename);

#endif

