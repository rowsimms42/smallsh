# smallsh

This shell implements a subset of features of well-known shells, such as bash.\n
It does the following:\n
-Provides a prompt for running commands\n
-Handles blank lines and comments (#)\n
-Provides expansion for the variable $$
-Executes 3 commands exit, cd, and status via code built into the shell
-Executes other commands by creating new processes using a function from the exec family of functions
-Support input and output redirection
-Support running commands in foreground and background processes
-Implements custom handlers for 2 signals, SIGINT and SIGTSTP
