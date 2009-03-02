#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "commands.h"

#define PROMPT "AT91> "

static int usage(char *prog_name)
{
    fprintf (stderr, "Usage: %s [scriptfiles...]\n", prog_name);
    exit (EXIT_FAILURE);
}

int main (int argc, char *argv[])
{
    int retval = 0;
    int i;

    if (argc >= 2 && strcmp (argv[1], "--help") == 0)
        usage(argv[0]);

    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            if (command_exec_file (argv[i])  < 0)
                retval = -1;
        }

        return retval < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    } 

    while (1) {
#ifdef USE_READLINE
            char *line = readline (PROMPT);
            add_history (line);
#else
            char line[1024];
            printf (PROMPT);
            fflush (stdout);

            fgets (line, sizeof (line), stdin);
#endif

            if (!line || strncmp (line, "quit", 4) == 0 || strlen (line) == 0)
                break;

            if (command_exec_str (line) < 0)
                retval = -1;

#ifdef USE_READLINE
            free (line);
#endif
    }
    return retval < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

