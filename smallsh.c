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
#define FALSE 0
#define TRUE 1

int status = 0;
volatile sig_atomic_t foreground_only_mode = FALSE;

void return_status(int wstatus) {
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", WTERMSIG(wstatus));
    }
    fflush(stdout);
}

void handle_SIGTSTP(int signo) {
    if (foreground_only_mode == FALSE) {
        char* msg = "entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, msg, 49);
        foreground_only_mode = TRUE;
    }
    else {
        char* msg = "exiting foreground-only mode\n";
        write (STDOUT_FILENO, msg, 29);
        foreground_only_mode = FALSE;
    }
}

void handle_SIGINT(int signo){
    char* message = "\ncaught sigint\n";
    write(STDOUT_FILENO, message, 38);
}

int check_background(char** args, int index){
    int r = 0;
    if (strcmp(args[index - 1], "&") == 0) {
        r= 1;
    }
    return r;
}

void fill_processes(pid_t *running_processes){
    int i;
    for (i=0; i< 100; i++){
        running_processes[i] = -20;
    }
}

int find_available_spot(const pid_t *running_processes){
    int i;
    for (i=0; i<100; i++){
        if (running_processes[i] == -20)
            return i;
    }
    return -1;
}

void kill_background_processes(pid_t *running_processes, const int *process_amount){
    int i;
    for (i=0; i < (*process_amount); i++) {
        if (running_processes[i] != -20) {
            kill(running_processes[i], SIGKILL);
        }
    }
}

void check_background_processes(pid_t *running_processes, int *process_amount){
    pid_t temp_proc_id = -20;
    int exit_value = -20;
    int i;
    for (i=0; i<100; i++){
        if (running_processes[i] != -20){
            temp_proc_id = waitpid(running_processes[i], &exit_value, WNOHANG);
            if (temp_proc_id > 0){
                printf("background pid %i has completed - ", temp_proc_id);
                fflush(stdout);
                running_processes[i] = -20;
                (*process_amount)--;
                if(WIFEXITED(exit_value)){
                    printf("exit status: %d\n", WEXITSTATUS(exit_value));
                } else{
                    printf("terminated by signal: %d\n", WTERMSIG(exit_value));
                }
                fflush(stdout);
            }
        }
    }
}

int parse_input(char *line, char *input, char *output, char **args) {
    /*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*/
    const char *delimiters = " \n";
    char *token = NULL;
    char *save_ptr = NULL;
    char *command = NULL;
    int index = 1;

    token = strtok_r(line, delimiters, &save_ptr);
    args[0] = token;

    do {
        token = strtok_r(NULL, delimiters, &save_ptr);

        if (token == NULL) {
            break;
        }
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, delimiters, &save_ptr);
            if (token != NULL) {
                sprintf(input, "%s", token);
                token = strtok_r(NULL, delimiters, &save_ptr);
                if (token == NULL) {
                    break;
                }
            } else {
                printf("error - no file provided\n");
                status = 1;
                return -1;
            }
        }
        if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, delimiters, &save_ptr);
            if (token != NULL) {
                sprintf(output, "%s", token);
                token = strtok_r(NULL, delimiters, &save_ptr);
                if (token == NULL) {
                    break;
                }
            } else {
                printf("error - no file provided\n");
                status = 1;
                return -1;
            }
        }
        args[index] = token;
        index++;
    } while (token != NULL);
    args[index] = NULL;
    return index;
}

int built_in_comm(char* str, char** args, char* command){
    const char *delimiters = " \n";
    char* save_ptr = NULL;
    char* token = NULL;
    command = strtok_r(str, delimiters, &save_ptr);
    args[0] = command;
    int ret = 0;

    if (command == NULL || (*(args[0]) == '#')){
        ret = 1;
    }
    else if ((strcmp(args[0], "status") == 0)) {
        ret = 3;
    }
    else if ((strcmp(args[0], "exit") == 0)){
        ret = 2; /*exit program*/
    }
    else if ((strcmp(args[0], "#") == 0)) {
        ret = 1;
    }
    else if (strcmp(args[0], "cd") == 0){
        token = strtok_r(NULL, delimiters, &save_ptr);
        if (token == NULL) {
            chdir(getenv("HOME"));
        }
        else {
            chdir(token);
        }
        ret = 1;
    }
    return ret;
}

void expand_variable(const char* str, char* buffer_changed, unsigned int l){
    int pid = getpid();
    int len = sizeof(pid)+1;
    char p_str[len];
    sprintf(p_str, "%d", pid);
    char* ptr = p_str;
    int i = 0;

    while (str[i] != 0) {
        char c = str[i];
        if (str[i] == '$') {
            char d = str[i + 1];
            if (d == '$') {
                strncat(buffer_changed, ptr, len);
                i++;
            } else {
                strncat(buffer_changed, &c, 1);
            }
        } else {
            strncat(buffer_changed, &c, 1);
        }
        i++;
    }
    strncat(buffer_changed, "\0", 1);
}

void loop() {
    struct sigaction action_z = {0}; /*sigstp*/
    action_z.sa_handler = handle_SIGTSTP;
    sigfillset(&action_z.sa_mask);
    action_z.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &action_z, NULL);

    struct sigaction action_c = {0}; /*sigint*/
    /*action_c.sa_handler = handle_SIGINT;*/
    action_c.sa_handler = SIG_IGN;
    /*action_c.sa_flags = 0;*/
    sigfillset(&(action_c.sa_mask));
    sigaction(SIGINT, &action_c, NULL);
    char** args = NULL;
    char buffer[500];
    char* buffer_changed = NULL;
    char* temp_buff = NULL;
    /*char** args;*/
    size_t buffer_length = 500;
    pid_t running_processes[100];
    fill_processes(running_processes);
    int process_amount = 0;
    int comm = 0;
    int parse_ret = 0;
    char devnull[] = "/dev/null";
    pid_t spawnpid = -5;

    while (1){
        char input[64] = {'\0'};
        char output[64] = {'\0'};
        bool background = false;
        bool in = false;
        bool out = false;
        fflush(stdout);
        char* command = NULL;
        buffer_changed = NULL;
        temp_buff = NULL;
        check_background_processes(running_processes, &process_amount);
        printf(":");
        fflush(stdout);
        fgets(buffer, 500, stdin);
        comm = -1;
        if (buffer[0]!='\0'){
            args = (char **) malloc(500 * sizeof(char*));
            buffer_changed = (char*)calloc(500, sizeof(char));
            buffer_length = 500;
            expand_variable(buffer, buffer_changed, buffer_length);
            temp_buff = (char*)calloc(500, sizeof(char));
            strcpy(temp_buff, buffer_changed);
            comm = built_in_comm(temp_buff, args, command);
            if (comm == 3) {
                return_status(status);
                fflush(stdout);
            }
            else if (comm == 0) {
                parse_ret = parse_input(buffer_changed, input, output, args);
                if (parse_ret != -1){
                    command = args[0];
                    int index = parse_ret;

                    if (check_background(args, index) == 1){
                        args[index - 1] = NULL;
                        background = true;
                    }

                    if(input[0]!='\0'){in = true;}
                    if(output[0]!='\0'){out = true;}

                    if (background == true && in == false) {
                        strcpy(input, devnull);
                    } else if (background == true && out == false) {
                        strcpy(output, devnull);
                    }

                    spawnpid = fork();
                    switch (spawnpid) {
                        case 0:
                            if (out == true) { /*output file exists*/
                                int d_fd;
                                if ((d_fd = creat(output, 0644)) < 0) {
                                    perror("Can't open output file");
                                    exit(1);
                                }
                                dup2(d_fd, STDOUT_FILENO);
                                close(d_fd);
                            }
                            if (in == true) {
                                int s_fd;
                                if ((s_fd = open(input, O_RDONLY, 0)) < 0) {
                                    perror("Can't open input file");
                                    exit(1);
                                }
                                dup2(s_fd, STDIN_FILENO);
                                close(s_fd);
                            }
                            if(background == false){
                                action_c.sa_handler = SIG_DFL;
                                sigaction(SIGINT, &action_c, NULL);
                            }
                            action_z.sa_handler = SIG_IGN;
                            sigaction(SIGTSTP, &action_z, NULL);

                            execvp(command, args);
                            perror("error");
                            status = 1;
                            exit(1);
                        case -1:
                            perror("fork error");
                            status = 1;
                            break;
                        default:
                            if (background == true && foreground_only_mode == FALSE) {
                                waitpid(spawnpid, &status, WNOHANG);
                                printf("background pid is %i\n", spawnpid);
                                fflush(stdout);
                                int spot = find_available_spot(running_processes);
                                if (spot >= 0) {
                                    running_processes[spot] = spawnpid;
                                    process_amount++;
                                } else {
                                    printf("cannot handle additional background process\n");
                                    fflush(stdout);
                                }
                            }
                            else {
                                waitpid(spawnpid, &status, 0);
                                if (WIFSIGNALED(status)) {
                                    return_status(status);
                                }
                            }
                    }

                }
                memset(&input[0], 0, sizeof(input));
                memset(&output[0], 0, sizeof(output));
            }
            free(buffer_changed);
            free(temp_buff);
            free(args);
            memset(&buffer[0], 0, sizeof(buffer));
            fflush(stdout);
            if (comm == 2) {
                kill_background_processes(running_processes, &process_amount);
                /*check_background_processes(running_processes, &process_amount);*/
                /*exit(0);*/
                return;
            }
        }
    }
}

int main(){
    loop();
    return 0;
}
