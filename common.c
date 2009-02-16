#include <stdio.h>
#include "common.h"

int progress (const char *reason, uint64_t upto, uint64_t total)
{
    static uint64_t last_upto;
    static uint64_t last_total;
    static int started = -1;
    int total_time;
    uint64_t print_upto;
    char *units;

    if (total <= 0)
        return -1;

    // starting a new one
    if (total != last_total || upto < last_upto || upto == 0) {
        last_upto = 0;
        last_total = total;
        started = mseconds ();
    }
    total_time = mseconds() - started;

    if (upto > total)
        upto = total;

    printf ("\r%s: %lld%%", reason, upto * 100 / total);
    print_upto = upto;
    if (print_upto > 4096) {
        print_upto /= 1096;
        units = "kB";
        if (print_upto > 4096) {
            print_upto /= 1096;
            units = "MB";
        }
    } else
        units = "B";
    printf (" %lld%s", print_upto, units);

    if (total_time > 0) {
        int val = upto * 1000 / total_time;
        int time_remaining = val ? (total - upto) / val : 0;

        if (val > 4096) {
            val /= 1096;
            units = "kB";
            if (val > 4096) {
                val /= 1096;
                units = "MB";
            }
        } else
            units = "B";

        printf (" %d%s/s", val, units);

        if (time_remaining > 3 && time_remaining < 10000)
            printf (" %ds remaining", time_remaining);

    }
    printf ("              \r");

    if (upto == total && last_upto != upto)
        printf ("\n");
    fflush (stdout);

    last_upto = upto;
    // finished this one
    if (upto == total) {
        last_upto = 0;
        last_total = 0;
    }

    return 0;
}

