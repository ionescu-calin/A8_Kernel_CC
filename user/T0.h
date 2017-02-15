#ifndef __T0_H
#define __T0_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

// define symbols for P1 entry point and top of stack
extern void (*entry_T0)(); 
extern uint32_t tos_T0;

#endif
