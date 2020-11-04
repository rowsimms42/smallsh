/*header file for smallsh.c*/

#ifndef SMALLSH_H
#define SMALLSH_H

void return_status(int);
void handle_SIGTSTP(int);
int built_in_comm(char*, char**, char*);
int check_background(char**, int);
int parse_input(char*, char*, char*, char**);
void expand_variable(const char*, char*);
void loop();
void kill_background_processes(pid_t*, const int*);
void check_background_processes(pid_t*, int*);

#endif
