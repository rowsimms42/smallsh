/* project 3 - smallsh*/
/* compile & run: gcc --std=gnu99 -o smallsh main.c */
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
#include <sys/wait.h>
#include <sys/types.h> /* pid_t, not used in this example*/
#include <unistd.h> /* getpid(), getppid() -- parent */
#define BUFFER_LENGTH 2048
#define ARGUMENT_MAX 512

struct command_line{
    char* command;
    char* arg[ARGUMENT_MAX];
    char* in_file;
    char* out_file;
    char* b_round;
    int* status;
    int* arg_size;
};

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
    while(line!=NULL){
        if (line->command!=NULL) {
            free(line->command);}
        //free((line->arg));
        if (line->in_file!=NULL) {
            free(line->in_file);}
        //if (line->out_file!=NULL) {free(line->out_file);}
        //if (line->b_round!=NULL) {free(line->b_round);}
        //if (line->status!=NULL) {free(line->status);}
        //free(line);
    }

}

void return_status(int wstatus){
    if(WIFEXITED(wstatus)){
        printf("exit status: %d\n", WEXITSTATUS(wstatus));
    } else{
        printf("terminated by signal: %d\n", wstatus);
    }

}

void change_directory(char* path, int change){
    printf("%s\n", path);
    switch(change){
        case(1): /*home*/
            printf("home\n");
            //chdir(<home>);
            break;
        case(2): /*path*/
            printf("path\n");
            //chdir(path);
            break;
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
    int i = 0;
    int check_in = 0;
    int check_out = 0;
    int* p;
    char* in_f = "<";
    char* out_f = ">";

    check_background(curr_line, line);

    char *token = strtok_r(curr_line, delimiters, &save_ptr);
    if (token != NULL) {
        line->command = (char*)calloc(strlen(token) + 1, sizeof(char));
        strcpy(line->command, token);
    }
    token = strtok_r(NULL, " ", &save_ptr);

    while (token!=NULL) {
        if (strcmp(token, in_f) == 0) {
            token = strtok_r(NULL, " ", &save_ptr);
            line->in_file = (char *) calloc(strlen(token) + 1, sizeof(char));
            strcpy(line->in_file, token);
            check_in++;
        }
        if (strcmp(token, out_f) == 0) {
            token = strtok_r(NULL, " ", &save_ptr);
            line->out_file = (char *) calloc(strlen(token) + 1, sizeof(char));
            strcpy(line->out_file, token);
            check_out++;
        }
        if (check_in == 0 && check_out == 0) {
            line->arg[i] = token;
            i++;
            p = &i;
            line->arg_size = (int *) calloc(sizeof(i) + 1, sizeof(int));
            line->arg_size = p;
        }
        token = strtok_r(NULL, " ", &save_ptr);
    }
    return line;
}

int main() {
    char buffer[BUFFER_LENGTH];
    char *argv[ARGUMENT_MAX];

    while (1) {
        printf(":");
        fflush(stdout);
        fgets(buffer, BUFFER_LENGTH, stdin);
        int p = strlen(buffer);
        if (p > 1) {
            struct command_line *c_line = parse_input(buffer);
            //print_struct(c_line);
            if (strcmp(c_line->command, "exit") == 0) {
                break;
            }
            if (strcmp(c_line->command, "#") == 0) {
                continue;
            }
            print_struct(c_line);
            free(c_line);
        }
    }
    return EXIT_SUCCESS;
}
