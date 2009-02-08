#ifndef COMMON_H
#define COMMON_H
#include <sys/time.h>
#include <time.h>

#ifndef min
#define min(x,y) ({ \
        typeof(x) _x = (x);       \
        typeof(y) _y = (y);       \
        _x < _y ? _x : _y; })
#endif

#ifndef max
#define max(x,y) ({ \
        typeof(x) _x = (x);       \
        typeof(y) _y = (y);       \
        _x > _y ? _x : _y; })
#endif


#define mseconds() ({ struct timeval _tv; \
        gettimeofday (&_tv, NULL); \
        _tv.tv_sec * 1000 + _tv.tv_usec / 1000;\
        })

#define MASTER_CLOCK (180000000/2)
#define PLL_LOCK_TIMEOUT        1000000

/* PLLA = 18.432MHz * (126 + 1) / 13 = 180.07 MHz */
#define PLLA_SETTINGS   ((1 << 29) | (126 << 16) | (2 << 13) | (0x3F << 8) | (13))

/* Set PLLB to 18.432 MHz * (119 + 1) / ( 23 * 2 ^ 1 ) = 48.08 MHz */
#define PLLB_SETTINGS   ((1 << 28) | (119 << 16) | (1 << 14) | (0x3f << 8) | 23)

/* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
#define MCKR_SETTINGS   (AT91C_PMC_CSS_PLLA_CLK | AT91C_PMC_PRES_CLK | AT91C_PMC_MDIV_2)

#define BAUDRATE(mck, baud) \
    (((((mck) * 10) / ((baud) * 16)) % 10) >= 5) ? \
        (mck / (baud * 16) + 1) : ((mck) / (baud * 16))

int progress (const char *reason, int upto, int total);

#endif /* COMMON_H */
