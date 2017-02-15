#include "T2.h"

void T2() {
  int pipefd1[2];
  int pipefd2[2];
  int pipefd3[2];

  if(pipe(pipefd1) == -1)
    write(1, "failed pipe\n", 14);
  else
    write(1, "success pipe\n", 14);
  if(pipe(pipefd2) == -1)
    write(1, "failed pipe\n", 14);
  else
    write(1, "success pipe\n", 14);
  if(pipe(pipefd3) == -1)
    write(1, "failed pipe\n", 14);
  else
    write(1, "success pipe\n", 14);
  //write_no3(pipefd1[0]);
    //write_no3(pipefd2[0]);
     // write_no3(pipefd3[0]);
  int pid1 = fork();
  if(pid1 == 0) {
    int pid2 = fork();
    if(pid2 == 0) {
      char z[100]; 
      for(int i = 0; i<= 5; i++) {
      while(!read(pipefd2[0], z, 1));    
      z[0] = z[0] + 1;
      write(pipefd3[1], z, 1);
      write_str3("P3: ");
      write(1, z, 1);
      write_str3("\n");
  	  }
    }
    else {
    char y[100];
    for(int i = 0; i<= 5; i++) {
    while(!read(pipefd1[0], y, 1));
    y[0] = y[0] + 1;
    write_str3("P2: ");
    write(pipefd2[1], y, 1);
    write(1, y, 1);
    write_str3("\n");
	}
    }
  }
  else {
  	char x[100] = "a";
  	for(int i = 0; i<= 5; i++) {
    write(pipefd1[1], x, 1);
    write_str3("P1: ");
    write(1, x, 1);
    write_str3("\n");
    while(!read(pipefd3[0], x, 1));
    x[0] = x[0] + 1;

	}
  }  

  exit();
}

void (*entry_T2)() = &T2;
