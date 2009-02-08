#include <stdio.h>
#include "common.h"

int progress (const char *reason, int upto, int total)
{
    static int last_upto = -1;
    static int last_total = -1;
    static int started = -1;
    int total_time;

    if (total <= 0)
        return -1;

    // starting a new one
    if (total != last_total || upto < last_upto || upto == 0) {
        last_upto = -1;
        last_total = total;
        started = mseconds ();
    }
    total_time = mseconds() - started;

    if (upto > total)
        upto = total;

    printf ("\r%s: %d%%", reason, upto * 100 / total);
    if (total_time > 0) {
        char *units = "B";
        int val = upto * 1000 / total_time;

        if (val > 4096) {
            val /= 1096;
            units = "kB";
            if (val > 4096) {
                val /= 1096;
                units = "MB";
            }
        }

        printf (" %d%s/s", val, units);
    }
    if (upto == total)
        printf ("\n");
    fflush (stdout);

    last_upto = upto;
    // finished this one
    if (upto == total) {
        last_upto = -1;
        last_total = -1;
    }

    return 0;
}

