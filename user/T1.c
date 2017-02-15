#include "T1.h"

void T1() {
  char y[100];
  char* x = y;
  read(0, x, 22);
  int i = 0;
  while(*x) {
  	if(*x>64&&*x<91){
  		*x = *x + 32;
  	}
  	i++;
  	x = x + 1;
  }
  write(1, y, 22);
  exit();
}

void (*entry_T1)() = &T1;
