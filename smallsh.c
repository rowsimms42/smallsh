/* project 3 - smallsh*/
/* compile & run: gcc --std=gnu99 -o smallsh smallsh.c*/
/* run with valgrind --leak-check=full ./smallsh */
/* run without valgrind ./smallsh */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h> /* getpid(), getppid() -- parent */
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#define BUFFER_LENGTH 2048
#define ARGUMENT_MAX 512

struct command_line{
    char* command;
    char* arg[ARGUMENT_MAX];
    char* in_file;
    char* out_file;
    char* b_round;
    int* arg_size;
    bool io_arr[2];
};

/* signal handler for SIGINT -- taken from lecture*/
void handle_SIGINT(int signo) {
    char *message = "Caught SIGINT, sleeping for 10 seconds\n";
    write(STDOUT_FILENO, message, 39);
    sleep(10);
}
/* signal handler for SIGSTP -- taken from lecture*/
void handle_SIGSTP(int signo) {
    char *message = "Caught SIGSTP\n";
    write(STDOUT_FILENO, message, 39);
    exit(0);
}


void print_struct(struct command_line* print_line){
    printf("command: %s\n",print_line->command);
    printf("in_file: %s\n",print_line->in_file);
    printf("out_file: %s\n",print_line->out_file);
    printf("b_round: %s\n",print_line->b_round);


    if (print_line->arg[0]!=NULL) {
        int x = (*print_line->arg_size);
        for (int i = 0; i < x; i++) {
            printf("arg: %s\n", print_line->arg[i]);
        }
    }
    fflush(stdout);
}

void free_struct(struct command_line* line){
    if(line){
        if (line->command) {
            free(line->command);
        }
        /*free((line->arg));*/
        if (line->in_file) {
            free(line->in_file);
        }
        if (line->out_file) {
            free(line->out_file);
        }
        if (line->b_round) {
            free(line->b_round);
        }

        free(line);
    }

}

void return_status(int wstatus){
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", wstatus);
    }

}

void change_directory(struct command_line* line) {
    if ((*line->arg_size) == 0) { /*home*/
        chdir(getenv("HOME"));
    } else { /*path*/
        chdir(line->arg[0]);
        printf("%s\n", line->arg[0]);

    }
}

void check_background(char* c_line, struct command_line* line){
    int len = strlen(c_line);
    char* pos;
    char* b = "&\n";
    pos = c_line+len-2;
    char* b_true = "T";
    char* b_false = "F";
    int num = 0;

    if (strcmp(pos, b) == 0){
        num = 1;
    }

    if (num == 1) {
        line->b_round = (char *) calloc(strlen(b_true) + 1, sizeof(char));
        strcpy(line->b_round, b_true);
    }
    else{
        line->b_round = (char *) calloc(strlen(b_false) + 1, sizeof(char));
        strcpy(line->b_round, b_false);
    }
}

struct command_line *parse_input(char *curr_line) {
    struct command_line* line= ((struct command_line*)malloc(sizeof(struct command_line)));
    char *save_ptr;
    char delimiters[] = " \n";
    int* p;
    int i = 0;
    int check_in = 0;
    int check_out = 0;
    char* in_f = "<";
    char* out_f = ">";
    line->io_arr[0] = false; /*input f*/
    line->io_arr[1] = false; /*output f*/

    check_background(curr_line, line);

    char *token = strtok_r(curr_line, delimiters, &save_ptr);
    if (token != NULL) {
        line->command = (char*)calloc(strlen(token) + 1, sizeof(char));
        strcpy(line->command, token);
        line->arg_size = (int *) calloc(sizeof(i) + 1, sizeof(int));
    }
    token = strtok_r(NULL, " ", &save_ptr);

    while (token!=NULL) {
        if (strcmp(token, in_f) == 0) {
            token = strtok_r(NULL, " ", &save_ptr);
            line->in_file = (char *) calloc(strlen(token) + 1, sizeof(char));
            line->in_file[0] = '\0';
            strcpy(line->in_file, token);
            check_in++;
            line->io_arr[0] = true;
        }
        if (strcmp(token, out_f) == 0) {
            token = strtok_r(NULL, " ", &save_ptr);
            line->out_file = (char *) calloc(strlen(token) + 1, sizeof(char));
            line->out_file[0] = '\0';
            strcpy(line->out_file, token);
            check_out++;
            line->io_arr[1] = true;
        }
        if (check_in == 0 && check_out == 0) {
            line->arg[i] = token;
            i++;
            p = &i;
            line->arg_size = p;
        }
        token = strtok_r(NULL, " ", &save_ptr);

    }
    return line;
}

int main() {
    char buffer[BUFFER_LENGTH];
    char *argv[ARGUMENT_MAX];
    int status = 0;
    int spawnpid;
    struct sigaction SIGINT_action = {0}, SIGSTP_action = {0}; /*Initialize structs to be empty*/
    /* Fill out the SIGINT_action struct*/
    SIGINT_action.sa_handler = handle_SIGINT; /* Register handle_SIGINT as the signal handler*/
    sigfillset(&SIGINT_action.sa_mask);   /* Block all catchable signals while handle_SIGINT is running*/
    SIGINT_action.sa_flags = 0; /* No flags set*/
    sigaction(SIGINT, &SIGINT_action, NULL);    /* Install our signal handler*/
    SIGSTP_action.sa_handler = handle_SIGSTP;
    SIGSTP_action.sa_flags = 0;
    sigfillset(&SIGSTP_action.sa_mask);
    SIGSTP_action.sa_flags = 0; /* No flags set*/
    sigaction(SIGTSTP, &SIGSTP_action, NULL);

    while (1) {
        printf(":");
        fflush(stdout);
        fgets(buffer, BUFFER_LENGTH, stdin);
        int p = strlen(buffer);
        if (p > 1) {
            struct command_line *c_line = parse_input(buffer);
            if (strcmp(c_line->command, "exit") == 0) {
                break;
            }
            if (strcmp(c_line->command, "#") == 0) {
                continue;
            }
            if (strcmp(c_line->command, "cd") == 0) {
                change_directory(c_line);
            } else {
                spawnpid = fork();
                switch (spawnpid) {
                    case -1:
                        perror("fork() failed!");
                        status = 1;
                        break;
                    case 0:
                        SIGINT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &SIGINT_action, NULL);
                        if (c_line->io_arr[0] == true) { /*input file exists*/
                            printf("infile: %s\n", c_line->in_file);
                            fflush(stdout);
                            int sourceFD = open(c_line->in_file, O_RDONLY);
                            if (sourceFD == -1) {
                                perror("source open()");
                                exit(1);
                            }
                            int result = dup2(sourceFD, 0);
                            if (result == -1) {
                                perror("source dup2()");
                                exit(2);
                            }
                            close(sourceFD);
                        }
                        if (c_line->io_arr[1] == true) { //*output file exists*/
                            int targetFD = open(c_line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            if (targetFD == -1) {
                                perror("target open()");
                                exit(1);
                            }
                            int result2 = dup2(targetFD, 1);
                            if (result2 == -1) {
                                perror("target dup2()");
                                exit(2);
                            }
                            close(targetFD);
                        }
                        //execv( args[0], args );

                        execvp(buffer[0], argv);
                        perror("execve\n");
                        fflush(stdout);
                        exit(1);


                        break;
                    default:

                        (void) waitpid(spawnpid, &status, 0);

                        break;
                }

            }
        }
        spawnpid = waitpid(-1, &status, WNOHANG);
        while(spawnpid > 0) {
            printf("background process, %i, is done: ", spawnpid);
            return_status(status);
            spawnpid = waitpid(-1, &status, WNOHANG);
            /*print_struct(c_line);*/

        }

    }
    return EXIT_SUCCESS;
}
