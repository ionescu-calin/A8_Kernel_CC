#include "P3.h"

void P3() {
  char x[100];

  int pipefd[2];
	if(pipe(pipefd) == -1)
		write(1, "failed pipe\n", 13);
	else
		write(1, "success dup\n", 12);

  int i = fork();

  if(i==0) {
		dup2(0, pipefd[0]);
		close(pipefd[0]);
		close(pipefd[1]);
	 }
	else {
		dup2(1, pipefd[1]);
		close(pipefd[0]);
		close(pipefd[1]);
	}



while(1) {
  //exit();

  if(i == 0) {
	read(0, x, 16);
	write(1, x, 16);
  }
   else {
    write(1, "I am the child\n", 16);
  }
}
  return;
}
void (*entry_P3)() = &P3;
