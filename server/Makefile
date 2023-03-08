# ECEN-5713 Advanced Embedded Software Development
# Author - Khyati Satta
# Date - 26 January 2023
# File Description - Makefile for the aesdsocket.c (Supports both cross-compilation and native compilation)

all: aesdsocket

CROSS_COMPILE = 
CC ?= $(CROSS_COMPILE)gcc

LDFLAGS?= -lpthread -lrt

aesdsocket: aesdsocket.c
	$(CC) aesdsocket.c $(LDFLAGS) -Wall -Werror -g -o aesdsocket

clean:
	rm -rf *.o aesdsocket 