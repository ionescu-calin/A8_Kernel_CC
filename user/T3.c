#include "T3.h"

void T3() {
  char y[100];
  char* x = y;
  read(0, x, 22);
  while(*x) {
  	if(*x== 'L' || *x == 'l'){
  		*x = '0';
  	}
  	x = x + 1;
  }
  write(1, y, 22);
  exit();
}


void (*entry_T3)() = &T3;
