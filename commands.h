#ifndef COMMANDS_H
#define COMMANDS_H

int command_exec (int argc, const char *argv[]);
int command_exec_str (char *command);
int command_exec_file (const char *filename);

#endif

