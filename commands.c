#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "at91sam.h"
#include "commands.h"
#include "common.h"
#include "at91sam.h"
#include "debug.h"
#include "nand.h"
#include "sdramc.h"
#include "pmc.h"
#include "boards.h"

static at91_t *at91 = NULL;

int command_exec (int argc, const char *argv[])
{
#if 0
    int i;
    printf ("%d args\n", argc);
    for (i = 0; i < argc; i++)
        printf ("%d: %s\n", i, argv[i]);
#endif

    if (strcasecmp (argv[0], "open") == 0) {
        const char *model = "sn9260";

        if (argc >= 2)
            model = argv[1];

        if (strcasecmp (model, "sn9260") == 0) {
            at91 = at91_open (0x3eb, 0x6124);
            if (!at91)
                return -1;
            return sn9260_init (at91);
        } else {
            fprintf (stderr, "Unknown AT91 baseboard model: %s\n", argv[1]);
            return -1;
        }
    } else if (strcasecmp (argv[0], "close") == 0) {
        at91_close (at91);
        at91 = NULL;
    } else if (strcasecmp (argv[0], "version") == 0) {
        char buffer[30];
        if (at91_version (at91, buffer, sizeof (buffer)) < 0)
            return -1;
        printf ("Version: %s\n", buffer);
    } else if (strcasecmp (argv[0], "go") == 0 && argc >= 2) {
        unsigned int addr = strtoul(argv[1], NULL, 0);
        at91_go (at91, addr);
    } else if (strcasecmp (argv[0], "readb") == 0 && argc >= 2) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        printf ("0x%8.8x = 0x%8.8x\n", addr, at91_read_byte (at91, addr));
    } else if (strcasecmp (argv[0], "readw") == 0 && argc >= 2) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        printf ("0x%8.8x = 0x%8.8x\n", addr, at91_read_half_word (at91, addr));
    } else if (strcasecmp (argv[0], "readl") == 0 && argc >= 2) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        printf ("0x%8.8x = 0x%8.8x\n", addr, at91_read_word (at91, addr));
    } else if (strcasecmp (argv[0], "read_file") == 0 && argc >= 4) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        const char *filename = argv[2];
        unsigned int length = strtoul (argv[3], NULL, 0);
        return at91_read_file (at91, addr, filename, length);
    } else if (strcasecmp (argv[0], "verify_file") == 0 && argc >= 3) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        const char *filename = argv[2];
        return at91_verify_file (at91, addr, filename);
    } else if (strcasecmp (argv[0], "writeb") == 0 && argc >= 3) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        unsigned int val = strtoul (argv[2], NULL, 0);
        return at91_write_byte (at91, addr, val);
    } else if (strcasecmp (argv[0], "writew") == 0 && argc >= 3) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        unsigned int val = strtoul (argv[2], NULL, 0);
        return at91_write_half_word (at91, addr, val);
    } else if (strcasecmp (argv[0], "writel") == 0 && argc >= 3) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        unsigned int val = strtoul (argv[2], NULL, 0);
        return at91_write_word (at91, addr, val);
    } else if (strcasecmp (argv[0], "write_file") == 0 && argc >= 3) {
        unsigned int addr = strtoul (argv[1], NULL, 0);
        const char *file = argv[2];
        return at91_write_file (at91, addr, file);
    } else if (strcasecmp (argv[0], "debug_init") == 0) {
        unsigned int baudrate = 115200;
        if (argc >= 2)
            baudrate = strtoul(argv[1], NULL, 0);
        dbg_init (at91, BAUDRATE(MASTER_CLOCK, baudrate));
    } else if (strcasecmp (argv[0], "debug_print") == 0) {
        int i;
        for (i = 1; i < argc; i++) {
            dbg_print (at91, argv[i]);
            if (i < argc - 1)
                dbg_print (at91, " ");
        }
    } else if (strcasecmp (argv[0], "nand_id") == 0) {
        printf ("0x%x\n", nand_read_id (at91));
    } else if (strcasecmp (argv[0], "nand_read_file") == 0 && argc >= 4) {
        unsigned int addr = strtoul(argv[1], NULL, 0);
        const char *file = argv[2];
        unsigned int length = strtoul(argv[3], NULL, 0);
        return nand_read_file (at91, addr, file, length);
    } else if (strcasecmp (argv[0], "nand_erase") == 0 && argc >= 3) {
        unsigned int addr = strtoul(argv[1], NULL, 0);
        unsigned int length = strtoul(argv[2], NULL, 0);
        return nand_erase (at91, addr, length);
    } else if (strcasecmp (argv[0], "nand_write_file") == 0 && argc >= 3) {
        unsigned int addr = strtoul(argv[1], NULL, 0);
        const char *file = argv[2];

        return nand_write_file (at91, addr, file);
    } else if (strcasecmp (argv[0], "nand_write_raw_file") == 0 && argc >= 3) {
        unsigned int addr = strtoul(argv[1], NULL, 0);
        const char *file = argv[2];

        return nand_write_raw_file (at91, addr, file);
    } else {
        fprintf (stderr, "Invalid command: %s\n", argv[0]);
        return -1;
    }

    return  0;
}

int command_exec_str (char *command)
{
    int nargs = 0;
    char *pos;
    char *args[20];

    pos = command;
    while (pos && *pos && nargs < 20) {
        if (*pos == '#') // comment
            break;
        args[nargs++] = pos;
        pos = strchr (pos, ' ');
        if (pos) {
            while (*pos == ' ') {
                *pos = '\0';
                pos++;
            }
        }
    }

    if (nargs > 0)
        return command_exec (nargs, args);

    return 0; // empty command
}

int command_exec_file (const char *filename)
{
    FILE *fp = fopen (filename, "rb");
    int retval = 0;
    char buffer[1024];
    char *pos;

    if (!fp) {
        fprintf (stderr, "Unable to open command file '%s': %s\n", filename, strerror (errno));
        return -1;
    }

    while (!feof (fp)) {
        if (!fgets (buffer, sizeof (buffer), fp))
            continue;

        pos = strrchr (buffer, '\r');
        if (pos)
            *pos = '\0';
        pos = strrchr (buffer, '\n');
        if (pos)
            *pos = '\0';

        if (strlen (buffer) == 0)
            continue;

        if (command_exec_str (buffer) < 0)
            retval = -1;
    }
    fclose (fp);

    return retval;
}

