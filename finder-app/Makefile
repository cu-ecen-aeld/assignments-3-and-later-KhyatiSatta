all: writer

CROSS_COMPILE = 
NATIVE = gcc

writer: writer.c
	$(CROSS_COMPILE)$(NATIVE) writer.c -Wall -Werror -g -o writer

clean:
	rm -rf *.o writer 