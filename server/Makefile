# ECEN-5713 Advanced Embedded Software Development
# Author - Khyati Satta
# Date - 26 January 2023
# File Description - Makefile for the aesdsocket.c (Supports both cross-compilation and native compilation)

all: aesdsocket

CROSS_COMPILE = 
CC = gcc

aesdsocket: aesdsocket.c
	$(CROSS_COMPILE)$(CC) aesdsocket.c -lrt -lpthread -Wall -Werror -g -o aesdsocket

clean:
	rm -rf *.o aesdsocket 