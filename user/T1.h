#ifndef __T1_H
#define __T1_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

// define symbols for P1 entry point and top of stack
extern void (*entry_T1)(); 
extern uint32_t tos_T1;

#endif
