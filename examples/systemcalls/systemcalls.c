#include "systemcalls.h"
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/  
    // The return status of the system shell command is stored to make error checks
    int ret_status = system(cmd);

    // Error check 1: If a child process could not be created, or it's status could not be retrieved
    if(ret_status == -1){
        printf("Error: system() function with error %d\n", errno);
        return false;
    }

    // Error check 2: If a shell could not be executed in the child process
    else if(ret_status == 127){
        return false;
    }
    else{
        return true;
    }
    
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    //Pid type variable for child pid
    pid_t child_pid;

    child_pid = fork();

    // Error check 1: Check if there was error in creating the child process
    if(child_pid == -1){ 
        perror("Child PID: fork() command");
        return false;
    }
    // Check if it is the child process
    else if (child_pid == 0){
        int ret_status = 0;
        ret_status = execv(command[0], command);
        // Error check 1`: Error in executing execv 
        if (ret_status == -1) {
            perror("Child PID: execv() command");
        }
        printf("Error in execv() with error: %d \n", errno);
        exit(1);
    }
    // If not child process 
    else{
        int ret_status = 0;
        pid_t wait_pid = waitpid(child_pid, &ret_status, 0);

        // Error check 1: If there was an issue with waiting pid
        if(wait_pid == -1){
            printf("Error executing command: waitpid with error: %d \n", errno);
            perror("Wait PID: wait() command");
            return false;
        } 
        else{
            // Error check 2: Check if the process was able to exit 
            if(WIFEXITED(ret_status)){
               // Error check 3: Check the exit status
               if(WEXITSTATUS(ret_status) != 0){
                return false;
               } 
               else {
                return true;
               }
            } 
            else {
                return false;
            }
        }      
    }
    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/  
    // Integer type variable for the child pid
    int child_pid;

    // Open the output file with write permission and create if not already created
    int file_descriptor = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644); 

    // Error check 1: Check if the file could be opened/created 
    if (file_descriptor == -1) {
        printf("Error opening/creating the file\n");
        perror("open");
        return false;
    }

    // Create new child process
    switch(child_pid = fork()) {
        // If there is an error
        case -1: 
            perror("Child PID:fork() command"); 
            return false;

        // If it is a child Process
        case 0: 
                if(dup2(file_descriptor, 1) < 0) { 
                perror("Child PID:dup2() command"); 
                exit(1); 
            }
            close(file_descriptor);
            execv(command[0], command); 
            perror("Child PID: execv() command"); 
            exit(1); 

        default:
            close(file_descriptor);
            int ret_status = 0;
            pid_t wait_pid = waitpid(child_pid, &ret_status, 0);

            if (wait_pid == -1) {
                perror("Wait PID:wait() command");
                return false;
            } else {
                // Check if process exited normally
                if (WIFEXITED (ret_status)) {
                    // Checking status of process
                    if (WEXITSTATUS(ret_status) != 0) {
                        return false;
                    } else {
                        return true;
                    }
                } else {
                    return false;
                }
            }
    }

    va_end(args);

    return true;
}