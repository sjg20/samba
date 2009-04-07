#ifndef COMMON_H
#define COMMON_H
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

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

extern unsigned int master_clock;
#define BAUDRATE(baud) \
    (((((master_clock) * 10) / ((baud) * 16)) % 10) >= 5) ? \
        (master_clock / (baud * 16) + 1) : ((master_clock) / (baud * 16))

int progress (const char *reason, uint64_t upto, uint64_t total);

#endif /* COMMON_H */
