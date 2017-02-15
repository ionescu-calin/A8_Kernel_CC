#ifndef __KERNEL_H
#define __KERNEL_H

#include <stddef.h>
#include <stdint.h>

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"
#include "file.h"

#include "interrupt.h"

// Include functionality from newlib, the embedded standard C library.

#include <string.h>

// Include definitions relating to the 3 user programs.

#include "P0.h"
#include "P1.h"
#include "P2.h"
#include "P3.h"
#include "T0.h"
#include "T1.h"
#include "T2.h"
#include "T3.h"
#include "shell.h"
/* The kernel source code is made simpler by three type definitions:
 *
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
     preservation and restoration prologue and epilogue,
 * - a type that captures a process identifier, which is basically an
 *   integer, and
 * - a type that captures a process PCB.
 */
#define stage 0                      //0, the initial stage with P0, P1 and P2 running (only works with RR scheduling), 1 the final stage
#define scheduler_type 0             //0 round robin, 2 sjf
#define fdNo 10                      //how many fdt are allowed for each process, >=2
#define pipeNo 10                    //max number of globaly opened pipes
#define fileNo 10                    //max number of globaly opened files
#define pipeBufferSize 100
#define maxProcs 100                 //max number of processes
#define maxProcesses maxProcs
#define lowestPriority 101           //100 is the lowest priority
#define max_path_length 100
#define def_priority 6
#define testPriority def_priority

typedef enum {DEAD, ACTIVE, WAITING, SCHEDULED} status_t;       //States of a process
typedef enum {PIPE, FILE, STDI, STDO} fdtype_t;                 //The types of file descriptors
typedef enum {READ, WRITE, READWRITE} fdpermissions_t;          //Permissions for files/pipes

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef int pid_t;

//---OFT RELATED STUFF

typedef struct pipe_t {
  char buffer[pipeBufferSize];
  int active;                 //pipe status (DEAD = 0/ACTIVE = 1)
  uint32_t fdCount;           //how many fds are linked to it
  uint32_t bufferIndex;       //amount of items in the buffer
} pipe_t;

typedef struct oft_t { //open file table contains either files or pipes
  pipe_t pipe[pipeNo];
  file_t file[fileNo];
  uint32_t fileCount, pipeCount; //No of files/pipes opened
  uint32_t used_file[fileNo];    //array of 0/1 to keep track of the active/dead files
  uint32_t used_pipe[pipeNo];    //array of 0/1 to keep track of the active/dead pipes
} oft_t;

//---END OF OFT RELATED STUFF

typedef struct fd_t {
    fdtype_t type;
    fdpermissions_t perm;
    uint32_t active;
    uint32_t fp;
    pipe_t* pipep;              //pointer to pipe in oft
    file_t* filep;              //pointer to file in oft
    uint16_t file_addr;         //address of the current dir, cached just to make fewer disk calls
    //char buffer[100]; //byte buffer? it is now int the oft pipe
} fd_t;

typedef struct {
  uint32_t pid;
  uint32_t priority;
} Stack;                        //priority stack used for scheduling


typedef struct {
  pid_t pid;
  pid_t ppid;                           //parent pid
  ctx_t ctx;
  uint32_t tos;                         //top of stack
  uint32_t stackLocation;               //where the process is in the priority stack
  uint32_t priority;
  status_t status;                      //Scheduled/dead/waiting
  fd_t fdt[fdNo];                       //file descriptor table
  char path[max_path_length];           //if it is a file, it will have a path name
} pcb_t;

#endif
