#ifndef DEBUG_H
#define DEBUG_H

void dbg_init(at91_t *at91, unsigned int baudrate);
void dbg_print(at91_t *at91, const char *ptr);
void dbg_hex(at91_t *at91, unsigned int val, int len);

#endif
