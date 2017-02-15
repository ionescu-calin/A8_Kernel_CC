#ifndef __LIBC_H
#define __LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// cooperatively yield control of processor, i.e., invoke the scheduler
void yield();

// write n bytes from x to the file descriptor fd
// returns -1 if fd doesn't have the right permissions
int write( int fd, void* x, size_t n );

//read n bytes from x to the file descriptor fd
// returns -1 if fd doesn't have the right permissions
int read( int fd, void* x, size_t n );

//creates an identical copy of the process
//returns 0 to child and pid of the child to the parent
int fork();

//compares two strings
int strcmp(const char* s1, const char* s2);

//converts string to int
int atoi(char* st);

//compares first n characters of the strings
int strncmp2(const char* s1, const char* s2, long n);

//reads n bytes and outputs them to STDOUT
int readAndWrite(void* x, size_t n);

//writes the entire string
void write_str3(char* s);

//replaces program with another program specified by the name
void exec(char* name);

//replaces the oldfd of the current process with newfd if newfd already exists else return 0 or -1 if fd out of bounds
//also closes newfd
//returns -1 fds are out of bounds
int dup2(int oldfd, int newfd);

// -1 fail, 1 success, 0 already closed
// if fd is the last file descriptor, the resources associated with the open pipe are freed;
//does not dealocate resources for files right now
int close(int fd);

//return 0 for success, -1 for fail (POSIX)
//Create the two fdt
//pipefd[0] - read; pipefd[1] - write
int pipe(int* pipefd);

//reads a line while printing it to the screen (reads untill ENTER)
void read_write_line(char* x);

//replaces program with another program specified by the name and sets its priority
void execp(char* name, int priority);

//exits a process
void exit();

//lists all the files within the current path of the process (to console)
void ls();

//change the current path of the process
//returns -1 if dir/path does not exist
int cd(char* name);

//creates dir
//returns -1 if dir already exists
int mkdir(char* name);

//returns the current path of a process
void pwd(char* path);

//opens or creates a file
//returns 0 for success
//returns -1 if file could not have been created/opened
//returns -2 if permissions(oflag) are not compatible
int open(char* path, int oflag );

//moves file pointer of a file descriptor by amount
//returns the new fp
int lseek(int fd, int amount);

//deletes a file from the file system
// returns 0 for success, -1 for fail
int unlink(const char *pathname);

//writes a string untill it encounters a \0
void write_no3(uint32_t n);

#endif
