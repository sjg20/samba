/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaiimer below.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the disclaimer below in the documentation and/or
 * other materials provided with the distribution.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 * File Name           : debug.c
 * Object              :
 * Creation            : ODi Apr 19th 2006
 *-----------------------------------------------------------------------------
 */
//#include "../include/part.h"
//#include "../include/main.h"

#include "at91sam.h"

//#ifdef CFG_DEBUG

#if 0
/* Write DBGU register */
static inline void write_dbgu(unsigned int offset, const unsigned int value)
{
	writel(value, offset + AT91C_BASE_DBGU);
}

/* Read DBGU registers */
static inline unsigned int read_dbgu( unsigned int offset)
{
	return readl(offset + AT91C_BASE_DBGU);
}
#endif

#define write_dbgu(o,v) at91_write_word(at91, AT91C_BASE_DBGU + o, v)
#define read_dbgu(o) at91_read_word(at91, AT91C_BASE_DBGU + o)


//*----------------------------------------------------------------------------
//* \fn    dbg_init
//* \brief This function is used to configure the DBGU COM port
//*----------------------------------------------------------------------------*/
void dbg_init(at91_t *at91, unsigned int baudrate)
{
        // turn on debug pio stuff
        at91_write_word (at91, PMC_PCER + AT91C_BASE_PMC, 1 << AT91C_ID_SYS);
        at91_write_word (at91, AT91C_BASE_PIOB + PIO_PDR(0), 1 << 15);
        at91_write_word (at91, AT91C_BASE_PIOB + PIO_ASR(0), 1 << 15);

	/* Disable interrupts */
	write_dbgu(US_IDR, -1);
	/* Reset the receiver and transmitter */
	write_dbgu(US_CR, AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS  | AT91C_US_TXDIS);
	/* Configure the baudrate */
	write_dbgu(US_BRGR, baudrate);
	/* Configure USART in Asynchronous mode */
	write_dbgu(US_MR, AT91C_US_PAR);
	/* Enable RX and Tx */
	write_dbgu(US_CR, AT91C_US_RXEN | AT91C_US_TXEN);
}

static void dbg_char(at91_t *at91, char ch) {
    while ( !(read_dbgu(DBGU_CSR) & AT91C_US_TXRDY) ); 
    write_dbgu(DBGU_THR, ch); 
}

//*----------------------------------------------------------------------------
//* \fn    dbg_print
//* \brief This function is used to configure the DBGU
//*----------------------------------------------------------------------------*/
void dbg_print(at91_t *at91, const char *ptr)
{
        while (*ptr)  {
            dbg_char (at91, *ptr++);
        }
}

void dbg_hex(at91_t *at91, unsigned int val, int len)
{
    const char *hex="0123456789ABCDEF";
    switch (len) {
        case 8: dbg_char(at91, hex[(val >> 28) & 0xf]);
        case 7: dbg_char(at91, hex[(val >> 24) & 0xf]);
        case 6: dbg_char(at91, hex[(val >> 20) & 0xf]);
        case 5: dbg_char(at91, hex[(val >> 16) & 0xf]);
        case 4: dbg_char(at91, hex[(val >> 12) & 0xf]);
        case 3: dbg_char(at91, hex[(val >> 8) & 0xf]);
        case 2: dbg_char(at91, hex[(val >> 4) & 0xf]);
        case 1: dbg_char(at91, hex[(val >> 0) & 0xf]);
    }
}

//#endif /* CFG_DEBUG */
