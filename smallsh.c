/******************
*Rowan Simmons
*Small shell in c
*11/3/20
********************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include "smallsh.h"
#define FALSE 0
#define TRUE 1
#define LINE_MAX 2048

/*global variables for status and foreground only mode*/
int status = 0;
volatile sig_atomic_t foreground_only_mode = FALSE;

/*return exit value of last process, with the exception of build in processes*/
void return_status(int wstatus) {
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", WTERMSIG(wstatus));
    }
    fflush(stdout);
}

/*signal handler for control-z. toggles foreground mode on and off*/
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
    fflush(stdout);
}

/*see if command is a background command (&)
 * returns to loop function*/
int check_background(char** args, int index){
    int r = 0;
    if (strcmp(args[index - 1], "&") == 0) {
        r= 1;
    }
    return r;
}

/* fill process array with junk values at start*/
void fill_processes(pid_t *running_processes){
    int i;
    for (i=0; i< 100; i++){
        running_processes[i] = -20;
    }
}

/*find next spot to kill array of running processes.
 * returns index number.*/
int find_available_spot(const pid_t *running_processes){
    int i;
    for (i=0; i<100; i++){
        if (running_processes[i] == -20)
            return i;
    }
    return -1;
}

/*upon exiting program, kill all running processes*/
void kill_background_processes(pid_t *running_processes, const int *process_amount){
    int i;
    for (i=0; i < (*process_amount); i++) {
        if (running_processes[i] != -20) {
            kill(running_processes[i], SIGKILL);
        }
    }
}

/*see if any background processes have finished, and if so, get exit value of process*/
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

/*parse string and store in arguments array. handles redirection.*/
int parse_input(char *line, char *input, char *output, char **args) {
    /*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*/
    const char *delimiters = " \n";
    char *token = NULL;
    char *save_ptr = NULL;
    int index = 1;

    /*tokenize string and find each word*/
    token = strtok_r(line, delimiters, &save_ptr);
    args[0] = token;

    do {
        token = strtok_r(NULL, delimiters, &save_ptr);

        if (token == NULL) {
            break;
        }
        if (strcmp(token, "<") == 0) { /*redirect input*/
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
        if (strcmp(token, ">") == 0) { /*redirect output*/
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
    return index; /*returns to loop function*/
}

/*parse string and see if command is a comment(#), cd, exit,
 * or status. return to loop function. ignores background command.*/
int built_in_comm(char* str, char** args, char* command){
    const char *delimiters = " \n";
    char* save_ptr = NULL;
    char* token = NULL;
    command = strtok_r(str, delimiters, &save_ptr);
    args[0] = command;
    int ret = 0;

    if (command == NULL || (*(args[0]) == '#')){ret = 1;}
    else if ((strcmp(args[0], "status") == 0)) {ret = 3;}
    else if ((strcmp(args[0], "exit") == 0)){ret = 2;} /*exit program*/
    else if ((strcmp(args[0], "#") == 0)) {ret = 1;}
    else if (strcmp(args[0], "cd") == 0){
        token = strtok_r(NULL, delimiters, &save_ptr);
        if (token == NULL) {chdir(getenv("HOME"));}
        else {chdir(token);}
        ret = 1;
    }
    return ret; /*returns to loop function*/
}

/* expands instances of $$ to process id.*/
void expand_variable(const char* str, char* buffer_changed){
    pid_t pid = getpid();
    int len = sizeof(pid_t)+2;
    char p_str[len];
    sprintf(p_str, "%i", pid); /*int to string*/
    char* ptr = NULL;
    ptr = p_str;
    int i = 0;

    /*traverse and build new string*/
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

/* loops until user enters enters exit.
 * provides user with prompt and takes user input.
 * holds structs for signals control-z and control-c.*/
void loop() {
    struct sigaction action_z = {0}; /*sigstp*/
    action_z.sa_handler = handle_SIGTSTP;
    sigfillset(&action_z.sa_mask);
    action_z.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &action_z, NULL);

    struct sigaction action_c = {0}; /*sigint*/
    action_c.sa_handler = SIG_IGN;
    sigfillset(&(action_c.sa_mask));
    sigaction(SIGINT, &action_c, NULL);

    char** args = NULL; /*argument holder*/
    char buffer[LINE_MAX]; /*user input*/
    char* buffer_changed = NULL;
    char* temp_buff = NULL;
    pid_t running_processes[100]; /*holds background children process id numbers*/
    fill_processes(running_processes); /*fill array with junk values*/
    int process_amount = 0; /*keep track of number of background child processes*/
    int comm;
    int parse_ret;
    char devnull[] = "/dev/null";
    pid_t spawnpid;

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
        /*check if any background processes finished*/
        check_background_processes(running_processes, &process_amount);
        printf(":");
        fflush(stdout);
        fgets(buffer, LINE_MAX, stdin);

        if (buffer[0]!='\0'){
            args = (char **) malloc(LINE_MAX * sizeof(char*));
            buffer_changed = (char*)calloc(500, sizeof(char));
            expand_variable(buffer, buffer_changed); /*expand instances of $$*/
            temp_buff = (char*)calloc(500, sizeof(char));
            strcpy(temp_buff, buffer_changed);
            comm = built_in_comm(temp_buff, args, command); /*handle build in commands*/
            if (comm == 3) { /*status*/
                return_status(status);
                fflush(stdout);
            }
            else if (comm == 0) { /*not a built in command*/
                /*parse and handle redirection*/
                parse_ret = parse_input(buffer_changed, input, output, args);
                if (parse_ret != -1){
                    command = args[0];
                    int index = parse_ret;
                    if (check_background(args, index) == 1){ /*background command*/
                        args[index - 1] = NULL;
                        background = true;
                    }
                    /*check if input/output files are present*/
                    if(input[0]!='\0'){in = true;}
                    if(output[0]!='\0'){out = true;}

                    /*redirect to /dev/null*/
                    if (background == true && in == false) {
                        strcpy(input, devnull);
                    } else if (background == true && out == false) {
                        strcpy(output, devnull);
                    }

                    /*spawn child processes to execute commands*/
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
                            if (in == true) { /*input file exists*/
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
                        default: /*execute based on status of background and foreground mode*/
                            if (background == true && foreground_only_mode == FALSE) {
                                waitpid(spawnpid, &status, WNOHANG);
                                printf("background pid is %i\n", spawnpid);
                                fflush(stdout);
                                int spot = find_available_spot(running_processes);
                                if (spot >= 0) {
                                    running_processes[spot] = spawnpid; /*add process to array*/
                                    process_amount++;
                                } else {
                                    printf("cannot handle additional background process\n");
                                    fflush(stdout);
                                }
                            }
                            else {
                                waitpid(spawnpid, &status, 0); /*execute normally*/
                                if (WIFSIGNALED(status)) {
                                    return_status(status);
                                }
                            }
                    }
                }
                /*clean up*/
                memset(&input[0], 0, sizeof(input));
                memset(&output[0], 0, sizeof(output));
            }
            /*end of loop. free memory*/
            free(buffer_changed);
            free(temp_buff);
            free(args);
            memset(&buffer[0], 0, sizeof(buffer));
            fflush(stdout);
            if (comm == 2) { /*exit program*/
                kill_background_processes(running_processes, &process_amount);
                return; /*returns to main*/
            }
        }
    }
}
/* main function. starts program. */
int main(){
    loop();
    return 0;
}
