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
/* gcc --std=gnu99 -o smallsh smallsh2.c */

struct arguments{
    char input[64];
    char output[64];
    bool in;
    bool out;
};

/*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*/

void parse_input(char *line, int length) {
    char **args = (char**)malloc(length * sizeof(char*));
    struct arguments* arg_struct= ((struct arguments*)malloc(sizeof(struct arguments)));
    const char *delimiters = " \n";
    char *command = NULL;
    char *token = NULL;
    char *save_ptr = NULL;
    arg_struct->in = false;
    arg_struct->out = false;
    char* curr = NULL;
    int index = 1;
    pid_t spawnpid;
    int status = 0;

    command = strtok_r(line, delimiters, &save_ptr);
    if (command == NULL) goto freemem;
    if ((strcmp(command, "#") == 0)) goto freemem;

    args[0] = command;

    if (strcmp(command, "cd") == 0) {
        token = strtok_r(NULL, delimiters, &save_ptr);
        if (token == NULL) {
            chdir(getenv("HOME"));
        } else {
            chdir(token);
        }
        goto freemem;
    }
    do {
        token = strtok_r(NULL, delimiters, &save_ptr);
        if (token == NULL){
            break;
        }
        if (strcmp(token, "<") == 0){
            token = strtok_r(NULL, delimiters, &save_ptr);
            if (token!=NULL){
                strcpy(arg_struct->input, token);
                arg_struct->in = true;
                token = strtok_r(NULL, delimiters, &save_ptr);
                if (token ==  NULL){
                    break;
                }
            }else {
                printf("no file provided\n");
                goto freemem;
            }
        }
        if (strcmp(token, ">") == 0){
            token = strtok_r(NULL, delimiters, &save_ptr);
            if (token!=NULL){
                strcpy(arg_struct->output, token);
                arg_struct->out = true;
                token = strtok_r(NULL, delimiters, &save_ptr);
                if (token ==  NULL){
                    break;
                }
            }else {
                printf("no file provided\n");
                goto freemem;
            }
        }
        args[index] = token;
        index++;
    } while (token!=NULL);
    args[index] = NULL;

    spawnpid = fork();
    switch (spawnpid) {
        case 0:
            if (arg_struct->out == true) { //*output file exists*/
                int d_fd;
                if ((d_fd = creat(arg_struct->output, 0644)) < 0) {
                    perror("Couldn't open output file - ");
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
                    perror("Couldn't open input file - ");
                    free(args);
                    free(line);
                    exit(1);
                }
                dup2(s_fd, STDIN_FILENO);
                close(s_fd);
            }
            execvp(command, args);
            perror("execvp");
            free(args);
            free(line);
            exit(1);
        case -1:
            perror("fork()");
            status = 1;
            break;
        default:
            waitpid(spawnpid, &status, 0);
            break;
    }

    freemem:
    free(args);
    free(arg_struct);
}

int main() {
    char *buffer = NULL;
    size_t max = 0;
    size_t buffer_length;
    while (1){
        printf(":");
        fflush(stdout);
        buffer_length = getline(&buffer, &max, stdin);
        parse_input(buffer, buffer_length);
        fflush(stdout);
    }
    return 0;
}