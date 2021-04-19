CC=clang
CFLAGS=-Wall -Wpedantic -Wextra -Werror

default: fat32

fat32: a4_main.o file_sys_32.o
	$(CC) $(CFLAGS) a4_main.o file_sys_32.o -o fat32 

run:
	make fat32 && ./fat32 ./a4image info

a4_main.o: a4_main.c file.h fat32.h
	$(CC) $(CFLAGS) -c a4_main.c

file_sys_32.o: file_sys_32.c file_sys_32.h file.h fat32.h
	$(CC) $(CFLAGS) -c file_sys_32.c

clean:
	rm -rf *.o && rm -rf fat32