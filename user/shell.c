#include "shell.h"


int token_number(char* string) {
  int sum = 1;
  for(int i = 0; i<strlen(string); i++) {
    if(string[i] == ' ') {
      sum++;
    }
  }
  return sum;
}

void shell() {
  char* x = "Hello and welcome, type !h to see the available commands\n";
  char* name = "Blueberry: ";
  char* inv  = "\nBlueberry: Invalid command!\n";
  char* fold_err = "Folder already exists\n";
  char* file_err = "File operation failed\n";
  char* del_err  = "\nError deleting file";
  char* tokens;
  int no_tokens;
  int file = -1;

  //char tokens[100][100] = strtok();
  write( 0, x, strlen(x));
  while( 1 ) {
  	write(1, "Blueberry: ", strlen(name));
    read_write_line (x);
    no_tokens = token_number(x);
    tokens = strtok(x, " ");

    if(strcmp(tokens, "run") == 0 && no_tokens == 2) {
      write(1, "\n", 1);
      write(1, "Starting: ", strlen("Starting: "));
      tokens = strtok(NULL, " ");
      write(1, tokens, strlen(tokens));
      write(1, "\n", 1);
      int pid = fork();
      if(pid == 0) {
        exec(tokens);
      }
    }
    else if (strcmp(tokens, "run") == 0 && no_tokens == 3) {
      char* p1 = strtok(NULL, " ");
      char* p2 = strtok(NULL, " ");
      int pid = fork();
      if(pid == 0) {
        int pipefd[2];
        if(pipe(pipefd) == -1)
          write(1, "failed pipe\n", 14);
        else
          write(1, "success pipe\n", 14);
        int pid2 = fork();
        if(pid2==0) {
          dup2(0, pipefd[0]);
          close(pipefd[0]);
          close(pipefd[1]);
          exec(p2);
         }
        else {
          dup2(1, pipefd[1]);
          close(pipefd[0]);
          close(pipefd[1]);
          exec(p1);
        }
      }
    }
    else if (strcmp(tokens, "cd")== 0 ) {
      tokens = strtok(NULL, " ");
      if(cd(tokens) == -1){
        write_str3("\nFolder does not exist");
      }
      write(1, "\n", 1);
    }
    else if (strcmp(tokens, "ls")== 0 ) {
      write(1, "\n", 1);
      ls();
      write(1, "\n", 1);
    }
    else if (strcmp(tokens, "lseek")== 0  && no_tokens == 2) {
      tokens = strtok(NULL, " ");
      int offs = atoi(tokens);
      lseek(file, offs);
      if(file == -1)
        write(1, file_err, strlen(file_err));
      write(1, "\n", 1);
    }
    else if (strcmp(tokens, "mkdir")== 0 && no_tokens == 2) {
      tokens = strtok(NULL, " ");
      if(mkdir(tokens) == -1)
        write(1, fold_err, strlen(fold_err));
      write(1, "\n", 1);
    }
    else if (!strcmp(tokens, "pwd")) {
      char path[100];
      char path2[100] = "root/";
      pwd(path);
      write(1, "\n", 1);
      strcat(path2, path);
      write(1, path2, strlen(path2));
      write(1, "\n", 1);
    }
    else if (strcmp(tokens, "open")== 0  && no_tokens == 3) {
      char* name = strtok(NULL, " ");
      char* type = strtok(NULL, " ");
      if(!strcmp(type, "w"))
        file  = open(name, 2);
      else if(!strcmp(type, "r"))
        file  = open(name, 1);
      else
        file  = open(name, 0);
      if(file == -1)
        write_str3(file_err);
      if(file == -2)
        write_str3("\nPermission denied");
      write(1, "\n", 1);
    }
    else if (!strcmp(tokens, "read")) {
      char s[1000];
      tokens = strtok(NULL, " ");
      if(file == -1)
        write(1, "Cannot read\n", 12);
      else
        if(read(file, s, atoi(tokens)) == -1)
          write_str3("\nPermission denied\n");
        else {
          write(1, "\n", 1);
          write(1, s, strlen(s));
          write(1, "\n", 1);
        }
    }
    else if(!strcmp(tokens, "write") && no_tokens >= 2) {
        tokens = strtok(NULL, "");
        if(file == -1)
          write_str3("\nNo file opened");
        write(1, "\n", 1);
        if(write(file, tokens, strlen(tokens)) == -1) {
          write_str3("\nPermission denied\n");
        }
    }
    else if(!strcmp(tokens, "unlink")) {
      tokens = strtok(NULL, " ");
      if(unlink(tokens) == -1)
        write(1, del_err, strlen(del_err));
      write(1, "\n", 1);
    }
    else if (strcmp(tokens, "runp") == 0 && no_tokens == 3) {
      char* pn;
      pn = strtok(NULL, " ");
      tokens = strtok(NULL, " ");
      write(1, "\n", 1);
      write(1, "Starting: ", strlen("Starting: "));
      write(1, pn, strlen(pn));
      write(1, "\n", 1);
      int pid = fork();
      if(pid == 0) {
        execp(pn, atoi(tokens));
      }
    }
    else {
    	write(1, inv, strlen(inv));
    }
  }
}

void (*entry_shell)() = &shell;
