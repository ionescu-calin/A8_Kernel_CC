#include "T0.h"

void T0() {
  char* x = "HELLO, this is a test\n";
  char* y = x;
  while(*x) {
  	if(*x>96&&*x<123){
  		*x = *x - 32;
  	}
  	x = x + 1;
  }
  write(1, y, 22);

  exit();
}

void (*entry_T0)() = &T0;
