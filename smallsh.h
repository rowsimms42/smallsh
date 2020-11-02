#ifndef SMALLSH_H
#define SMALLSH_H
#include <signal.h>
#include <sys/types.h>


void return_status(int);
void handle_SIGTSTP(int);
void handle_SIGINT(int);
int built_in_comm(char*, char**, char*);
int check_background(char**, int);
int parse_input(char*, char*, char*, char**);
void expand_variable(const char*, char*, unsigned int);
void loop();
void kill_background_processes(pid_t*, const int*);
void check_background_processes(pid_t*, int*);

#endif
