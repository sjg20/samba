#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "environment.h"
#include "crc32.h"

#define BUFLEN 1024

//#define DEBUG

#define CRC_SIZE 4

static struct {
    char *data;
    unsigned long size;
} _local = {
    .data = NULL,
    .size = 0,
};

void environment_dump (void)
{
    char *buffer = &_local.data[CRC_SIZE];
    while (buffer && *buffer) {
        printf ("%s\n", buffer);
        buffer += strlen (buffer) + 1;
    }
}

static void cleanup (void)
{
    if (_local.data) {
        free (_local.data);
        _local.data = NULL;
    }
}

int environment_init (int length)
{
    if (_local.data)
        free (_local.data);
    _local.data = malloc (length);
    _local.size = length;
    return 0;
}

int environment_data (char **data, int *length)
{
    unsigned int *data32 = (unsigned int *)_local.data;
    unsigned int crc;
    crc = crc32(0, &_local.data[CRC_SIZE], _local.size - CRC_SIZE);
    *data32 = crc;
    *data = _local.data;
    *length = _local.size;
    return 0;
}

static char *find_var_pos (const char *var)
{
    char *curpos = &_local.data[CRC_SIZE];
    int varlen = strlen (var);

    if (!curpos)
        return NULL;

    while (*curpos != '\0' && *curpos != '\xff') {
        int len = strlen (curpos);

        assert (len > 0);

        if (strncmp (curpos, var, varlen) == 0 &&
                curpos[varlen] == '=') // found it
            return curpos;

        curpos += len + 1; // move to next string
    }

    return NULL;
}

int environment_get_raw (const char *var, char *result, int max_len)
{
    const char *retval = NULL;

    if (!_local.data)
        return -1;

    retval = find_var_pos (var);
    if (retval)
        strncpy (result, retval, max_len);

    return retval ? 0 : -1;
}

int environment_get (const char *var, char *result, int max_len)
{
    char buffer[200];
    if (environment_get_raw (var, buffer, sizeof (buffer)) < 0) {
        *result = '\0';
        //err ("Unable to find variable '%s' in environment", var);
        return -1;
    }
    strncpy (result, &buffer[strlen(var)+1], max_len);
    return 0;
}

int environment_clear (const char *var)
{
    char *var_pos = find_var_pos (var);
    int len, remaining;
    char *end;

    if (!var_pos)
        return 0; // already removed

#ifdef DEBUG
    printf ("Clearing variable '%s'\n", var);
#endif

    len = strlen (var_pos);
    end = var_pos + len + 1;
    remaining = _local.data + _local.size - end;

    memmove (var_pos, end, remaining);

    return 0;
}

static int append_var (const char *var, const char *value)
{
    char buffer[BUFLEN];
    sprintf (buffer, "%s=%s", var, value);
    int buflen = strlen (buffer);
    char *curpos = &_local.data[CRC_SIZE];

    if (!curpos)
        return -1;

#ifdef DEBUG
    printf ("Appending var '%s' [%d]\n", buffer, buflen);
#endif

    while (*curpos != '\0' && *curpos != '\xff') {
        int len = strlen (curpos);
        assert (len > 0);

        curpos += len + 1; // move to next string
    }
    memmove (curpos, buffer, buflen);
    curpos[buflen] = '\0';
    curpos[buflen + 1] = '\0';

    return 0;
}

/**
 * Expand variables & quoted characters etc...
 */
static int value_expand(const char *value, char *result, int max_len)
{
    const char *pos = value;
    char prev = '\0';
    char *dest = result;
    char in_quote = '\0';

    while (pos && *pos) {
        if (*pos == '$' && prev != '\\' && !in_quote) {
            const char *var_pos = pos + 1;
            char variable_name[256];
            char *up_to = variable_name;
            char variable_value[1024];

            while (*var_pos && strchr (" -:\r\n\t'\";\\", *var_pos) == NULL) {
                *up_to++ = *var_pos++;
                pos++;
            }
            *up_to++ = '\0';
            environment_get (variable_name, variable_value, sizeof (variable_value));
            //printf ("Replacing '%s' with '%s'\n", variable_name, variable_value);
            strcpy (dest, variable_value);
            dest += strlen (variable_value);
        } else {
            if (*pos == '\\' && prev != '\\') // escaping the next char
                ;
            else {
                if (prev != '\\') {
                    if (*pos != '\"' && *pos != '\'')
                        *dest++ = *pos;

                    //if (*pos == '\"') {
                    //if (!in_quote)
                    //in_quote = '\"';
                    //else if (in_quote == '\"')
                    //in_quote = 0;
                    //}

                    if (*pos == '\'') {
                        if (!in_quote)
                            in_quote = '\'';
                        else if (in_quote == '\'')
                            in_quote = 0;
                    }
                } else
                    *dest++ = *pos;
            }
            //printf ("pos=%c in_Quote=%c\n", *pos, in_quote);
        }

        prev = *pos;
        pos++;
    }
    *dest++ = '\0';

    return 0;
}

int environment_set (const char *var, const char *value)
{
    //printf ("Changing variable '%s' to '%s'\n", var, value);

    if (environment_clear (var) < 0) // empty it first
        return -1;

    if (value && *value) // a null value is the same as clearing it
        if (append_var (var, value) < 0)
            return -1;

    return 0;
}

static int read_until (FILE *fp, char *destination, char *matches)
{
    int read = 0;
    while (!feof (fp)) {
        if (fread (destination, 1, 1, fp) != 1) {
            perror ("Unable to read from file");
            return -1;
        }
        read++;
        if (strchr (matches, *destination)) {
            *destination = '\0';
            break;
        }
        destination++;
    }

    return read;
}

static int read_word (FILE *fp, char *buffer)
{
    return read_until (fp, buffer, " \r\n\t;");
}

static int read_value (FILE *fp, char *buffer)
{
    int len;
    char quote = 0;
    char *pos, *upto;
    char tmp[4192];
    int trailing_slash;
    char prev = '\0';

    // Keep reading lines until we don't finish one inside a quoted region
    upto = tmp;
    do {
        if (fgets (upto, 1024, fp) == NULL) {
            perror ("Unable to read");
            return -1;
        }
        //printf ("Read %s", upto);
        for (pos = upto; *pos; pos++) {
            if (prev != '\\' && *pos == '\'') {
                if (quote == 0)
                    quote = *pos;
                else if (quote == *pos)
                    quote = 0;
            }
            if (prev != '\\' && *pos == '\"') {
                if (quote == 0)
                    quote = *pos;
                else if (quote == *pos)
                    quote = 0;
            }
            prev = *pos;
        }
        len = strlen(upto);
        if (len) {
            trailing_slash = (upto[len - 2] == '\\');
            if (trailing_slash)
                upto[len - 2] = '\n';
        } else
            trailing_slash = 0;
        upto += len;
    } while (quote != 0 || trailing_slash != 0);

    value_expand (tmp, buffer, 1024);

    return strlen (buffer);
}

static int skip_whitespace (FILE *fp)
{
    char ch;
    int count = 0;
    char line[1024];
    int len;

    do {
        len = fread (&ch, 1, 1, fp);
        if (len < 0) {
            perror ("EOF");
            return -1;
        } else if (len == 0)
            break;
        count++;
        if (ch == '#') // Skip to eol
            fgets (line, sizeof (line), fp);
        else if (!strchr (" \r\n\t", ch)) {
            fseek (fp, -1, SEEK_CUR);
            break;
        }
    } while (1);

    return count;
}


int environment_update_from_file (const char *filename)
{
    FILE *fp;
    char buffer[BUFLEN];
    char variable[BUFLEN];
    char value[BUFLEN];

    if ((fp = fopen (filename, "r")) == NULL) {
        perror ("Unable to open environment file");
        return -1;
    }

    while (!feof (fp))
    {
        int len;

        if (skip_whitespace (fp) < 0)
            goto err;

        memset (buffer, 0, sizeof (buffer));
        if (read_word (fp, buffer) == 0)
            break;

        if (strcmp (buffer, "setenv") != 0) {
            fprintf (stderr, "Invalid command: '%s'\n", buffer);
            goto err;
        }
        if (skip_whitespace (fp) < 0)
            goto err;
        if (read_word (fp, variable) < 0)
            goto err;
        if (read_value (fp, value) < 0)
            goto err;
        len = strlen (value);

        while (len > 0 && (value [len - 1] == '\n' || value[len - 1] == '\r'))
        {
            len--;
            value[len] = '\0';
        }
        environment_set(variable, value);
    }

    fclose (fp);
    return 0;

err:
    fclose (fp);
    return -1;
}
