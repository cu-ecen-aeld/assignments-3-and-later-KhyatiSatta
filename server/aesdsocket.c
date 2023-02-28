/*ECEN-5713 Advanced Embedded Software Development
Author - Khyati Satta
Date - 24 February 2023
File Description - Socket Programming
References: Beej's guide and lecture material
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

#define BACKLOG   (20u)
// #define  DEBUG     (0u)
#define BUFFER_SIZE  (512u)

// Flag to free the contents of the buffer and handling open connections
volatile uint8_t sig_flag = 0;

// The file and socket descriptors are global for being to able take action in the signal handler
int sd = 0;
int fd = 0;
int new_fd;

char *write_buffer;

void signalHandler(int signal) 
{
    switch(signal) {

        case SIGINT: 
        syslog(LOG_DEBUG ,"Caught signal SIGINT, exiting\n");
        free(write_buffer);
        if(close(sd) == -1){
            syslog(LOG_ERR , "close sd\n");
        }
        if(close(fd) == -1){
            syslog(LOG_ERR , "close fd\n");
        }
        if(close(new_fd) == -1){
            syslog(LOG_ERR , "close new fd\n");
        }
        if(unlink("/var/tmp/aesdsocketdata") == -1){
            syslog(LOG_ERR , "unlink file\n");
        }
        break;

        case SIGTERM:
        syslog(LOG_DEBUG ,"Caught signal SIGTERM, exiting\n");
        free(write_buffer);
        if(close(sd) == -1){
            syslog(LOG_ERR , "close sd\n");
        }
        if(close(fd) == -1){
            syslog(LOG_ERR , "close fd\n");
        }
        if(close(new_fd) == -1){
            syslog(LOG_ERR , "close new fd\n");
        }
        if(unlink("/var/tmp/aesdsocketdata") == -1){
            syslog(LOG_ERR , "unlink file\n");
        }
        break;
    }

    exit(EXIT_SUCCESS);

}

int createDaemon()
{
    int ret_status = 0;

    pid_t pid;

    // Fork a new child process
    pid = fork ();
    // Error check
    if (pid == -1){
        syslog(LOG_ERR , "fork\n");
        return -1;
    }
    else if (pid != 0){
        exit (EXIT_SUCCESS);
    }
        
    // Next step of creating a daemon is to begin a session 
    ret_status = setsid();   
    if (ret_status == -1){
        syslog(LOG_ERR , "setsid\n");
        return -1;
    }
        
    // Switch to the root directory
    ret_status = chdir("/");
    if (ret_status == -1){
        syslog(LOG_ERR , "chdir\n");
        return -1;
    }
        
    // Redirect the stdin , stdout and stderror to /dev/null
    open ("/dev/null", O_RDWR);
    dup (0); 
    dup (0); 

    return 0;
}

int main(int argc , char *argv[])
{
    openlog(NULL , 0 , LOG_USER);

    // Register the signals 
    signal(SIGINT , signalHandler);
    signal(SIGTERM , signalHandler);

    // Flag to track the if daemon is to be created
    uint8_t daemon_flag = 0;

    // Check if the -d argument was passed
    char *daemon_arg = argv[1];
    if(daemon_arg != NULL){
        if (strcmp(daemon_arg , "-d") == 0){
        daemon_flag = 1;
        }
        else{
            printf("Please enter the correct argument '-d' for creating the daemon\n");
            daemon_flag = 0;
        }
    }
    

    // Variable to store the return status
    int ret_status = 0;
    struct addrinfo *servinfo;
    struct addrinfo hints;

    // File to write the data from the socket
    fd = open("/var/tmp/aesdsocketdata" , O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    // Error check
    if (fd == -1){
        syslog(LOG_ERR , "open\n");
        return -1;
    }
    // printf("Value of fd:%d\n", fd);

    // Variable to use for reusing the port logic
    int check_port_status = 1;

    // The structure has to empty initially
    memset(&hints, 0, sizeof(hints));

    // Set the attributes of the hints variable
    hints.ai_family = AF_INET;     // IPv4 type of connection
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // Fill in the IP
 

    // Get the addrinfo structure for the port 9000
    if ((ret_status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0) {
        syslog(LOG_ERR , "getaddrinfo\n");
        return -1;
    }

    // Get the socket descriptor
    sd = socket(servinfo->ai_family , servinfo->ai_socktype , servinfo->ai_protocol);
    // Error check
    if(sd == -1){
        syslog(LOG_ERR , "socket\n");
        return -1;
    }

    ret_status = setsockopt(sd , SOL_SOCKET , SO_REUSEADDR , &check_port_status , sizeof(check_port_status));
    // Error check
    if (ret_status == -1){
        syslog(LOG_ERR , "setsocketopt\n");
        return -1;
    }

    // Bind the socket to the local port
    ret_status = bind(sd , servinfo->ai_addr , servinfo->ai_addrlen);
    // Error check
    if (ret_status == -1){
        syslog(LOG_ERR , "bind\n");
        return -1;
    }

    // Before listening for a new connection, after binding to port 9000, check if daemon is to be created
    if (daemon_flag){
        ret_status = createDaemon();

        if (ret_status == -1){
            syslog(LOG_ERR , "createDaemon\n");
        }
        else{
            syslog(LOG_DEBUG , "Daemon created suceesfully\n");
        }
    }

    ret_status = listen(sd , BACKLOG);
    if (ret_status == -1){
        perror("listen");
        return -1;
    }
#ifdef DEBUG
    struct sockaddr client_addr;
    socklen_t addr_size = sizeof(client_addr);

    new_fd = accept(sd , &client_addr, &addr_size);
    // Error check 
    if (new_fd == -1){
        perror("accept");
        return -1;
    }

    // Print IP address
    // The inet_toa function requires a sockaddr_in structure
    // Reference: https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
    struct sockaddr_in * ip_client = (struct sockaddr_in *)&client_addr;
    syslog(LOG_INFO , "Accepted connection from %s\n", inet_ntoa(ip_client->sin_addr));
    
    // Unit test to see if receive works from the sockettest.sh
    char buffer[10];

    int no_bytes = recv(new_fd , buffer , sizeof(buffer) , 0);

    printf("Printing the number of received characters: %d\n", no_bytes);
    printf("Error code: %d\n", errno);

    for(int i = 0; i < no_bytes; i++){
        printf("%c", buffer[i]);
    }
#endif
    
    freeaddrinfo(servinfo);

    // Logic to receive data packets

    struct sockaddr client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Declare a static buffers: One to get data from recv() 
    char recv_buffer[BUFFER_SIZE];

    // Malloc buffer of the same size initially for sending the data over to the client and writing to teh file
    write_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    // Error check: If malloc failed
    if (write_buffer == NULL){
        syslog(LOG_ERR , "malloc\n");
        return -1;
    }

    // Flag to check if \n was received
    uint8_t newline_flag = 0;
    int byte_count = 0;
    int datapacket = 0;
    int final_size = 1;
    int realloc_int = 0;
    static int file_size = 0;

    // While any signal has not been encountered yet
    while(1){

        new_fd = accept(sd , &client_addr, &addr_size);
        // Error check 
        if (new_fd == -1){
            syslog(LOG_ERR , "accept\n");
            // Try waiting for a connect() request from another remote
            continue;
        } 

        // Print IP address
        // The inet_toa function requires a sockaddr_in structure
        // Reference: https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
        struct sockaddr_in * ip_client = (struct sockaddr_in *)&client_addr;
        syslog(LOG_INFO , "Accepted connection from %s\n", inet_ntoa(ip_client->sin_addr));  

        while(!newline_flag){
            byte_count = recv(new_fd , recv_buffer , BUFFER_SIZE , 0);
            // printf("Byte count:%d\n", byte_count);
            if ( byte_count == -1){
                syslog(LOG_ERR , "recv\n");
                return -1;
            }
            else{
                final_size++;
                for(datapacket = 0; datapacket < BUFFER_SIZE; datapacket++){
                    if(recv_buffer[datapacket] == '\n'){
                        newline_flag = 1;
                        break;
                    }
                }

                memcpy((write_buffer + (BUFFER_SIZE * realloc_int)) , recv_buffer , BUFFER_SIZE);
                // printf("Buffer:%s\n", write_buffer);

                if(!newline_flag){
                    write_buffer = (char *)realloc(write_buffer , (sizeof(char) * (BUFFER_SIZE * final_size)));
                    // If realloc fails
                    if (write_buffer == NULL){
                        syslog(LOG_ERR , "realloc\n");
                    }
                    else{
                        realloc_int++;
                    }                    
                }
            }
        }

        // Reset the flag
        newline_flag = 0;

        // Now the data packet is acquired

        // Write the data packet to the file
        int buffer_len = ((datapacket + 1) + (BUFFER_SIZE * realloc_int));
        int write_len = write(fd , write_buffer , buffer_len);
        // Error check
        if(write_len == -1){
            syslog(LOG_ERR , "write\n");
            return -1;
        }
        // printf("Write len:%d\n", write_len);

        file_size += write_len;

        // printf("Size: %d\n", file_size);

        // Reset the reallocation tracking flag and the final size tracking flag
        realloc_int = 0, final_size = 1;

        // Allocate memory for the send buffer
        char *send_buffer = (char *)malloc(sizeof(char) * file_size);
        // Error check
        if(send_buffer == NULL){
            syslog(LOG_ERR , "malloc\n");
            return -1;
        }

        // Go to the beginning of the file using lseek
        off_t seek_status = lseek(fd , 0 , SEEK_SET);
        // Error check
        if(seek_status == -1){
            syslog(LOG_ERR , "lseek\n");
            return -1;
        }

        // Read the contents of the file
        ret_status = read(fd , send_buffer , file_size);
        // Error check
        if(ret_status == -1){
            syslog(LOG_ERR , "read\n");
            return -1;
        }

        // Send the buffer to the client
        ret_status = send(new_fd , send_buffer , file_size , 0);
        if(ret_status == -1){
            syslog(LOG_ERR , "send\n");
            return -1;
        }

        // Free the send buffer
        free(send_buffer);

        // Reallocate the buffer to original size
        write_buffer = realloc(write_buffer , BUFFER_SIZE);

        // Close the socket descriptor
        close(new_fd);

        syslog(LOG_INFO , "Closed connection from %s\n", inet_ntoa(ip_client->sin_addr));

    }

    // End logging
    closelog();

    
    return 0;
}


