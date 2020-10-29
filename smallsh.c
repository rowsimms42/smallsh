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

struct arguments {
    char input[64];
    char output[64];
    bool in;
    bool out;
    bool background;
};
int allowBackground = 0;

int status = 0; /*global status variable*/

void return_status(int wstatus) {
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", wstatus);
    }
}
/*
void catchSIGTSTP(int signo) {
    if (allowBackground == 1) {
        char* message = "Entering foreground-only mode (& is now ignored)\n";
        write(1, message, 49);
        fflush(stdout);
        allowBackground = 0;
    }
    else {
        char* message = "Exiting foreground-only mode\n";
        write (1, message, 29);
        fflush(stdout);
        allowBackground = 1;
    }
}
*/
/* gcc --std=gnu99 -o smallsh smallsh.c */

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

    struct sigaction action = {0};

    // Redirect ^Z to catchSIGTSTP()
    struct sigaction sa_sigtstp = {0};
    sa_sigtstp.sa_handler = catchSIGTSTP;
    sigfillset(&sa_sigtstp.sa_mask);
    sa_sigtstp.sa_flags = 0;
    sigaction(SIGTSTP, &sa_sigtstp, NULL);

    command = strtok_r(line, delimiters, &save_ptr);

    if (command == NULL) {
        goto freemem;
    }
    if ((strcmp(command, "#") == 0)) {
        goto freemem;
    }
    if ((strcmp(command, "status") == 0)) {
        return_status(status);
        goto freemem;
    }
    if ((strcmp(command, "exit") == 0)){
        exit(0);
    }

    args[0] = command;
    if (strcmp(command, "cd") == 0) {
        token = strtok_r(NULL, delimiters, &save_ptr);
        if (token == NULL) {
            chdir(getenv("HOME"));
        } else {
            int s = chdir(token);
            if (s==-1){
                printf("%s: Directory does not exist\n", token);
                status = 1;
                fflush(stdout);
            }
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
            perror(command);
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
    fflush(stdout);
}

char* extract(char* str, int length){
    char temp_str[length];
    char p_str[length+2];
    int pid = getpid();
    sprintf(p_str, "%d", pid);
    int len = sizeof(pid);
    int i = 0;
    char s = '$';
    char* ret_str = NULL;

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
    ret_str = (char*)malloc(sizeof(ret_str) * sizeof(char*));
    strcpy(ret_str, temp_str);
    str=NULL;
    memset(&temp_str[0], 0, sizeof(temp_str));
    return ret_str;
}

void loop() {
    char buffer[500];
    char* buffer_changed;
    size_t buffer_length = 500;
    while (1){
        printf(":");
        fflush(stdout);
        fgets(buffer, 500, stdin);
        buffer_changed = extract(buffer, buffer_length);
        parse_input(buffer_changed, buffer_length);
        fflush(stdout);
        buffer_changed = NULL;
    }
}

int main(){
    loop();
    return 0;
}
