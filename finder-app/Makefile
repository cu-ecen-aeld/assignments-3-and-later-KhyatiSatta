# ECEN-5713 Advanced Embedded Software Development
# Author - Khyati Satta
# Date - 26 January 2023
# File Description - Makefile for the writer script (Supports both cross-compilation and native compilation)

all: writer

CROSS_COMPILE = 
NATIVE = gcc

writer: writer.c
	$(CROSS_COMPILE)$(NATIVE) writer.c -Wall -Werror -g -o writer

clean:
	rm -rf *.o writer 