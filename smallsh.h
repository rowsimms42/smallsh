#ifndef SMALLSH_H
#define SMALLSH_H

struct arguments {
    char* input;
    char* output;
    bool in;
    bool out;
    bool background;
};
struct children {
    char child_pid;
    struct children* next;
};

void return_status(int);
void signal_SIGTSTP(int);
struct arguments* init_struct();
int change_directories(char**);
int built_in_comm(char**);
int check_background(char**, int);
void parse_input(char*, int);
void extract(char*, char*, int);
void loop();

#endif
