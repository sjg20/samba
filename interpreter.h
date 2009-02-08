#ifndef INTERPRETER_H
#define INTERPRETER_H

int interpreter_init (void);
int interpreter_close (void);
int interpreter_process_line (const char *line);
int interpreter_process_file (const char *filename);

#endif

