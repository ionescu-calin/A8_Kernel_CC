#ifndef __P3_H
#define __P3_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

// define symbols for P0 entry point and top of stack
extern void (*entry_P3)();
extern uint32_t tos_P3;

#endif
