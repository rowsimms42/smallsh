# smallsh

This shell implements a subset of features of well-known shells, such as bash.
It does the following:

-Provides a prompt for running commands

-Handles blank lines and comments (#)

-Provides expansion for the variable $$

-Executes 3 commands exit, cd, and status via code built into the shell

-Executes other commands by creating new processes using a function from the exec family of functions

-Supports input and output redirection

-Supports running commands in foreground and background processes

-Implements custom handlers for 2 signals, SIGINT and SIGTSTP


To compile and run: 

gcc -std=gnu99 -o smallsh smallsh.c

./smallsh
