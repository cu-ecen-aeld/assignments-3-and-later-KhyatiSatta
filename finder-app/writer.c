/*ECEN-5713 Advanced Embedded Software Development
Author - Khyati Satta
Date - 26 January 2023
File Description - The following application is to write a given expression to a file using File IO operations
*/

#include<stdio.h>
#include<syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<string.h>

#define ERROR_CODE (-1u)

//Function to write the given expression to the file
int fileWrite(int file_descriptor , char *file_name , char *expr)
{
    syslog(LOG_DEBUG , "Writing %s to %s ", expr , file_name);

    // Write to the file using write system call
    int error_code = write(file_descriptor , expr , strlen(expr));

    // Error check 1: Check if the write operation was successful
    // It will return -1 if the write operation was interrupted
    if(error_code == -1){
        syslog(LOG_ERR , "The write operation for the string %s to the file %s was unsucessful!\n" , expr , file_name);
        return 1;
    }
    // Error check 2: Check if all the bytes were written
    else if(error_code != strlen(expr)){
        syslog(LOG_ERR , "All the bytes were not written to the file. Only %d were written\n" , error_code);
        return 1;
    }

    // Close the file
    close(file_descriptor);

    return 0;
}


// Main function
int main(int argc , char *argv[])
{
    // To initiate the logging, setup utility to use with LOG_USER
    openlog(NULL , 0 , LOG_USER);

    // Error check 1: Check if the number of arguments are 2
    if(argc != 3){
        printf("Invalid number of arguments: %d. Please enter both the arguments\n",argc);
        syslog(LOG_ERR , "Invalid number of arguments: %d. Please enter both the arguments\n",argc);
        return 1;
    }

    // Variable to hold the file descriptor value
    int file_descriptor = 0;

    // Open the file with Write-only and Create attributes flags
    // The user, group as well as the owner have read, write and execute permissions
    file_descriptor = open(argv[1] , (O_WRONLY | O_CREAT) , (S_IRWXU | S_IRWXG | S_IRWXO));

    // Error check 2: Check if the file exists. This is achieved by opening the file and checking if there
    // is any error in the file descriptor
    if(file_descriptor == -1){
        printf("The file %s does not exist.Please create the file\n" , argv[1]);
        syslog(LOG_ERR , "The file %s does not exist. Please create the file\n", argv[1]);
    }

    // This function writes the desired expression to the file
    if(0 != fileWrite(file_descriptor , argv[1] , argv[2])){
        syslog(LOG_ERR , "The file write operation was unsucessful\n");
        printf("The file write operation was unsucessful\n");
    }

    // Close the logging functionality
    closelog();

    return 0;
}
