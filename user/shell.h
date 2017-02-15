#ifndef __shell_H
#define __shell_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "libc.h"

// define symbols for P2 entry point and top of stack
extern void (*entry_shell)(); 
extern uint32_t tos_shell;

#endif
