#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h> /* getpid(), getppid() -- parent */
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include "smallsh.h"


int status = 0;
int fg_mode = 1;

void return_status(int wstatus) {
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", wstatus);
    }
}

void signal_SIGTSTP(int signo) {
    if (fg_mode == 1) {
        char* msg = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, msg, 49);
        fg_mode = 0;
    }
    else {
        char* msg = "Exiting foreground-only mode\n";
        write (STDOUT_FILENO, msg, 29);
        fg_mode = 1;
    }
}

struct arguments* init_struct(){
    struct arguments* curr_arg = malloc(sizeof(struct arguments));
    curr_arg->in = false;
    curr_arg->out = false;
    curr_arg->background = false;
    return curr_arg;
}

int change_directories(char** token){
    int ret = 0;
    if (*token == NULL) {chdir(getenv("HOME"));}
    else {
        int s = chdir(*token);
        if (s == -1){
            printf("%s: Directory does not exist\n", *token);
            //status = 1;
            fflush(stdout);
            ret = 1;
        }
    }
    return ret;
}

int built_in_comm(char** args){
    int ret = 0;
    if ((strcmp(args[0], "status") == 0)) {
        return_status(status);
        ret = 1;
    }
    else if ((strcmp(args[0], "exit") == 0)){ret = 2;}
    else if ((strcmp(args[0], "#") == 0)) {ret = 1;}
    else if (strcmp(args[0], "cd") == 0){ret = 3;}
    return ret;
}

int check_background(char** args, int index){
    int ret = 0;
    if (strcmp(args[index - 1], "&") == 0) {
        args[index - 1] = NULL;
        ret = 1;
    }
    return ret;
}

void parse_input(char *line, int length) {
    /*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*/
    char **args = (char **) malloc(length * sizeof(char *));
    struct arguments *arg_struct = init_struct();
    const char *delimiters = " \n";
    char *command = NULL;
    char *token = NULL;
    char *save_ptr = NULL;
    int index = 1;
    pid_t spawnpid;

    struct sigaction sa_sigtstp = {0};
    sa_sigtstp.sa_handler = signal_SIGTSTP;
    sigfillset(&sa_sigtstp.sa_mask);
    sa_sigtstp.sa_flags = 0;
    sigaction(SIGTSTP, &sa_sigtstp, NULL);

    command = strtok_r(line, delimiters, &save_ptr);
    args[0] = command;

    if (command == NULL){goto freemem;}
    int comm = built_in_comm(args);
    if (comm == 1){goto freemem;}
    else if (comm == 2){exit(0);}
    else if (comm == 3){
        token = strtok_r(NULL, delimiters, &save_ptr);
        int ch_dir = change_directories(&token);
        if (ch_dir == 1) {
            status = 1;
        }
        goto freemem;
    }

    do {
        token = strtok_r(NULL, delimiters, &save_ptr);
        if (token == NULL) {
            break;
        }
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, delimiters, &save_ptr);
            if (token != NULL) {
                arg_struct->input = calloc(strlen(token) + 1, sizeof(char));
                strcpy(arg_struct->input, token);
                arg_struct->in = true;
                token = strtok_r(NULL, delimiters, &save_ptr);
                if (token == NULL) {
                    break;
                }
            } else {
                printf("no file provided\n");
                goto freemem;
            }
        }
        if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, delimiters, &save_ptr);
            if (token != NULL) {
                arg_struct->output = calloc(strlen(token) + 1, sizeof(char));
                strcpy(arg_struct->output, token);
                arg_struct->out = true;
                token = strtok_r(NULL, delimiters, &save_ptr);
                if (token == NULL) {
                    break;
                }
            } else {
                printf("no file provided\n");
                goto freemem;
            }
        }
        args[index] = token;
        index++;
    } while (token != NULL);
    args[index] = NULL;

    if (check_background(args, index) == 1){
        arg_struct->background = true;
    }

    char devnull[] = "/dev/null";

    if (arg_struct->background == true && arg_struct->in == false) {
        arg_struct->input = calloc(strlen(devnull) + 1, sizeof(char));
        strcpy(arg_struct->input, devnull);
    } else if (arg_struct->background == true && arg_struct->out == false) {
        arg_struct->output = calloc(strlen(devnull) + 1, sizeof(char));
        strcpy(arg_struct->output, devnull);
    }

    if (*(args[0]) == '#') {
        goto freemem;
    }

    spawnpid = fork();
    switch (spawnpid) {
        case 0:
            if (arg_struct->out == true) { //*output file exists*/
                int d_fd;
                if ((d_fd = creat(arg_struct->output, 0644)) < 0) {
                    perror("Couldn't open output file");
                    free(args);
                    free(line);
                    exit(1);
                }
                dup2(d_fd, STDOUT_FILENO);
                close(d_fd);
            }
            if (arg_struct->in == true) {
                int s_fd;
                if ((s_fd = open(arg_struct->input, O_RDONLY, 0)) < 0) {
                    perror("Couldn't open input file");
                    free(args);
                    free(line);
                    exit(1);
                }
                dup2(s_fd, STDIN_FILENO);
                close(s_fd);
            }
            execvp(command, args);
            perror("error()");
            free(args);
            free(line);
            exit(1);
        case -1:
            perror("fork()");
            status = 1;
            break;
        default:
            if (arg_struct->background == false || fg_mode == 0) {
                waitpid(spawnpid, &status, 0);
            } else {
                if (fg_mode == 1) {
                    printf("Background pid is %i\n", spawnpid);
                    fflush(stdout);
                }
            }
            spawnpid = waitpid(-1, &status, WNOHANG);
            while (spawnpid > 0) {
                printf("background process, %i, is done: ", spawnpid);
                fflush(stdout);
                return_status(status);
                spawnpid = waitpid(-1, &status, WNOHANG);
            }
    }
    freemem:
    free(args);
    free(arg_struct);
}

void extract(char* str, char* buffer_changed, int length){
    char temp_str[length];
    char p_str[length+2];
    int pid = getpid();
    sprintf(p_str, "%d", pid);
    int len = sizeof(pid);
    int i = 0;
    char s = '$';

    while (str[i] != 0) {
        char c = str[i];
        if (str[i] == '$') {
            char d = str[i + 1];
            if (d == s) {
                strncat(temp_str, p_str, len);
                i++;
            } else {
                strncat(temp_str, &c, 1);
            }
        } else {
            strncat(temp_str, &c, 1);
        }
        i++;
    }
    strncat(temp_str, "\0", 1);
    strcpy(buffer_changed, temp_str);
    memset(&temp_str[0], 0, sizeof(temp_str));
    return;
}

void loop() {
    char buffer[500];
    char* buffer_changed;
    size_t buffer_length = 500;
    while (1){
        printf(":");
        fflush(stdout);
        fgets(buffer, 500, stdin);
        buffer_changed = NULL;
        if (buffer[0]!='\0'){
            buffer_changed = malloc(sizeof(buffer) * sizeof(char));
            extract(buffer, buffer_changed, buffer_length);
            parse_input(buffer_changed, buffer_length);
            free(buffer_changed);
            memset(&buffer[0], 0, sizeof(buffer));
        }
    }
}

int main(){
    loop();
    return 0;
}
