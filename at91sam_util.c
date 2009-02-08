#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "at91sam.h"
#include "interpreter.h"
#include "nand.h"
#include "sdramc.h"
#include "pmc.h"
#include "debug.h"
#include "common.h"

static int usage(char *prog_name)
{
    fprintf (stderr, "Usage: %s -s script\n", prog_name);
    exit (EXIT_FAILURE);
}

int main (int argc, char *argv[])
{
    const char *short_opt = "s:";
    struct option long_opt[] = { 
        {"script", 1, 0, 's'},
        {0, 0, 0, 0},
    };
    const char *script = NULL;

    while (1) {
        int c = getopt_long (argc, argv, short_opt, long_opt, NULL);

        if (c == -1)
            break;

        switch (c) {
            case 's': script = optarg; break;
            default: usage(argv[0]); break;
        }
    }

    if (!script || optind < argc)
        usage(argv[0]);

    if (interpreter_init () < 0)
        return EXIT_FAILURE;
    interpreter_process_file (script);
    interpreter_close ();

    return 0;
}

#if 0 // old test main
static void dump_buffer (unsigned char *buffer, int length)
{
    int i;
    for (i = 0; i < length; i++)
        printf ("%2.2x%c", buffer[i], i % 16 == 15 ? '\n' : ' ');
    printf ("\n");
}

int main (int argc, char *argv[])
{
    at91_t *at91;
    char version[200];
    char *data = "This is some dummy data";
    char command[] = "write 0x3b foo";
    char buffer[2048 + 64] = {'\0'};

#if 1
    if (interpreter_init () < 0)
        return EXIT_FAILURE;
    interpreter_process_file ("./example.lua");
    interpreter_close ();

    return 0;
#endif
    at91 = at91_open (0x3eb, 0x6124);

    if (!at91)
        return EXIT_FAILURE;

    if (at91_version (at91, version, sizeof (version)) < 0)
        return EXIT_FAILURE;

    printf ("Version: %s\n", version);

#if 1
    if (nand_init (at91) < 0)
        return EXIT_FAILURE;

    printf ("NAND ID: 0x%x\n", nand_read_id (at91));

    if (nand_read_sector (at91, 0x20000, buffer, ZONE_DATA | ZONE_INFO) < 0)
        return EXIT_FAILURE;
    dump_buffer (buffer, 2048 + 64);
    if (nand_read_file (at91, 0x20000, "/home/andre/u-boot.bin", 0x40000) < 0)
        return EXIT_FAILURE;
#endif

    /* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
    pmc_cfg_plla(at91, PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
    pmc_cfg_mck(at91, MCKR_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Configure PLLB */
    pmc_cfg_pllb(at91, PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Initialize the matrix */
    writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) | AT91C_EBI_CS1A_SDRAMC, AT91C_BASE_CCFG + CCFG_EBICSA);


    /* Configure SDRAM Controller */
    sdram_init (at91,
                AT91C_SDRAMC_NC_9  |
                AT91C_SDRAMC_NR_13 |
                AT91C_SDRAMC_CAS_2 |
                AT91C_SDRAMC_NB_4_BANKS |
                AT91C_SDRAMC_DBW_32_BITS |
                AT91C_SDRAMC_TWR_2 |
                AT91C_SDRAMC_TRC_7 |
                AT91C_SDRAMC_TRP_2 |
                AT91C_SDRAMC_TRCD_2 |
                AT91C_SDRAMC_TRAS_5 |
                AT91C_SDRAMC_TXSR_8,            /* Control Register */
                (MASTER_CLOCK * 7)/1000000,     /* Refresh Timer Register */
                AT91C_SDRAMC_MD_SDRAM);         /* SDRAM (no low power)   */

    at91_write_word (at91, 0x20000000, 0xdeadbeef);
    printf ("@ 0x20000000: 0x%x\n", at91_read_word (at91, 0x20000000));
    at91_write_word (at91, 0x20000000, 0x12345678);
    printf ("@ 0x20000000: 0x%x\n", at91_read_word (at91, 0x20000000));

    dbg_init (at91, BAUDRATE (MASTER_CLOCK, 115200));
    dbg_print (at91, "At91SAM util debug startup\r\n");

#if 0
    printf ("@ 0x200000: 0x%x\n", at91_read_byte (at91, 0x200000));
    printf ("@ 0x200000: 0x%x\n", at91_read_half_word (at91, 0x200000));
    printf ("@ 0x200000: 0x%x\n", at91_read_word (at91, 0x200000));

    at91_write_word (at91, 0x200000, 0xdeadbeef);
    printf ("@ 0x200000: 0x%x\n", at91_read_word (at91, 0x200000));

    at91_write_word (at91, 0x200000, 0xea000020);
    printf ("@ 0x200000: 0x%x\n", at91_read_word (at91, 0x200000));

    if (at91_write_data (at91, 0x200000, (unsigned char *)data, strlen (data)) < 0)
        return EXIT_FAILURE;
    printf ("@ 0x200000: 0x%x\n", at91_read_word (at91, 0x200000));
    if (at91_read_data (at91, 0x200000, (unsigned char *)version, strlen(data)) < 0)
        return EXIT_FAILURE;
    version[strlen(data)] = '\0';
    printf ("@ 0x200000: '%s'\n", version);

    if (at91_write_file (at91, 0x200000, "/home/andre/snapper/utility/Bootstrap-v1.9/board/sn9260/sn9260_bootstrap.bin") < 0)
        return EXIT_FAILURE;
    if (at91_write_file (at91, 0x200000, "/home/andre/foo.bin") < 0)
        return EXIT_FAILURE;
    if (at91_read_file (at91, 0x200000, "/home/andre/output.bin", 4096) < 0)
        return EXIT_FAILURE;
#endif

    at91_close (at91);

    return EXIT_SUCCESS;;
}

#endif
