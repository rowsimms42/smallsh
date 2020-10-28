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
/* gcc --std=gnu99 -o smallsh smallsh.c */

struct arguments {
    char input[64];
    char output[64];
    bool in;
    bool out;
    bool background;
};

int status = 0; /*global status variable*/

char* expand(char** ptr){
    int i = 0;
    int pid = getpid();
    int len = sizeof(pid);
    char str[64];
    str[0] = 0;
    char p_str[len+2];
    sprintf(p_str, "%d", pid);
    len = len+2;
    char s = '$';

    while (((*ptr)[i]) !=0) {
        char c = (*ptr)[i];
        if (c == s) {
            char d = (*ptr)[i + 1];
            if (d == s) {
                strncat(str, p_str, len);
                i++;
            } else {
                strncat(str, &c, 1);
            }
        } else {
            strncat(str, &c, 1);
        }
        i++;
    }
    char **ret_str = (char**)malloc((len*i) * sizeof(char*));
    ret_str[0] = str;
    return *ret_str;
}

void arg_expansion(char **arg, int length) {
    char* substring = "$$";
    char* n = NULL;
    int i;
    char* new = NULL;

    for (i = 0; arg[i]!=NULL; i++) {
        n = strstr(arg[i], substring);
        if (n != NULL) {
            new = expand(&arg[i]);
            arg[i] = new;
        }
    }
    for (int i = 0; arg[i] != NULL; i++) {
        printf("%s ", arg[i]);
    }
    printf("\n");
}

void return_status(int wstatus) {
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", wstatus);
    }
}

void parse_input(char *line, int length) {
    /*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*/
    char **args = (char**)malloc(length * sizeof(char*));
    struct arguments* arg_struct= ((struct arguments*)malloc(sizeof(struct arguments)));
    const char *delimiters = " \n";
    char *command = NULL;
    char *token = NULL;
    char *save_ptr = NULL;
    arg_struct->in = false;
    arg_struct->out = false;
    arg_struct->background = false;
    char* curr = NULL;
    int index = 1;
    pid_t spawnpid;

    command = strtok_r(line, delimiters, &save_ptr);
    if (command == NULL) {goto freemem;}
    else if ((strcmp(command, "#") == 0)) {goto end;}
    else if ((strcmp(command, "status") == 0)) {
        return_status(status);
        goto freemem;
    }
    else if ((strcmp(command, "exit") == 0)) {
        exit(0);
        goto freemem;
    }
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

    if (strcmp(args[index-1],"&") == 0){
        arg_struct->background = true;
        args[index-1] = NULL;
    }

    if (*(args[0]) == '#'){
        goto freemem;
    }
    arg_expansion(args, length);


    spawnpid = fork();
    switch (spawnpid) {
        case 0:
            if (arg_struct->out == true) { //*output file exists*/
                int d_fd;
                if ((d_fd = creat(arg_struct->output, 0644)) < 0) {
                    perror("Couldn't open output file");
                    free(args);
                    free(line);
                    status = 1;
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
            perror("execvp");
            free(args);
            free(line);
            exit(1);
            break;
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
    end:
    //print_args(args);
    fflush(stdout);
}

int main() {
    char *buffer;
    size_t max = 0;
    size_t buffer_length;
    int background = 0;
    while (1){
        buffer = NULL;
        printf(":");
        fflush(stdout);
        buffer_length = getline(&buffer, &max, stdin);
        parse_input(buffer, buffer_length);
        fflush(stdout);
    }
    return 0;
}
