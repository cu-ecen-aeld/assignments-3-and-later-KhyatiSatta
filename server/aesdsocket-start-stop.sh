#ECEN-5713 Advanced Embedded Software Development
#Author name: Khyati Satta
#Date: 01/22/2023
#File Description: Startup script for aesdsocket
#Reference: Lecture material

#! /bin/sh

case "$1" in
    start)
        echo "Starting aesdsocket"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket"
        start-stop-daemon -K -n aesdsocket 
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac 

exit 0
