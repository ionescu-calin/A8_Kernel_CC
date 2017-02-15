#ifndef __T2_H
#define __T2_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

// define symbols for P1 entry point and top of stack
extern void (*entry_T2)(); 
extern uint32_t tos_T2;

#endif
