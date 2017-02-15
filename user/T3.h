#ifndef __T3_H
#define __T3_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

// define symbols for P1 entry point and top of stack
extern void (*entry_T3)(); 
extern uint32_t tos_T3;

#endif
