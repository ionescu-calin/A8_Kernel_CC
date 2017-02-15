#include "libc.h"
//

void yield() {
  asm volatile( "svc #0     \n"  );
}

int write( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #1     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int read( int fd, void* x, size_t n ) {
  int r=0;
  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #2     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int dup2(int oldfd, int newfd) {
	int r;
   asm volatile("mov r0, %1 \n"
					      "mov r1, %2 \n"
					      "svc #7     \n"
					      "mov %0, r0 \n"
				      : "=r" (r)
              : "r" (oldfd), "r" (newfd)
				      : "r0", "r1");
	return r;
}

int pipe(int* pipefd) {
  int r;
  asm volatile( "mov r0, %1 \n"
					      "svc #6    \n"
					      "mov %0, r0 \n"
				      : "=r" (r)
              : "r" (pipefd)
				      : "r0");
  return r;
}

void exec(char* name) {
  int r;
  asm volatile( "mov r0, %1 \n"
					      "svc #5    \n"
				      : "=r" (r)
              : "r" (name)
				      : "r0");
}

void execp(char* name, int priority) {
  int r;
  asm volatile("mov r0, %1 \n"
               "mov r1, %2 \n"
               "svc #0x09    \n"
              : "=r" (r)
              : "r" (name), "r" (priority)
              : "r0");
}

int readAndWrite(void* x, size_t n) {
	int sum = 0, step;
	while(sum != (int) n) {
		step = read(0, x, n);
		write(1,x,step);
		sum += step;
		x += step;
	}
  return 0;
}

void read_write_line(char* x) {
  int sum = 0, step;
  while(*(x-1) != '\r') {
    step = read(0, x, 1);
    write(1,x,step);
    sum += step;
    x += step;
  }
  *(x-1) = '\0';
}

int fork() {
  int r=0;

  asm volatile( "svc #3    \n"
                "mov %0, r0\n"
              : "=r" (r) );
  return r;
}

int close(int fd) {
  int r;
  asm volatile( "mov r0, %1 \n"
					      "svc #8     \n"
				      : "=r" (r)
              : "r" (fd)
				      : "r0");
  return r;
}

int strcmp(const char* s1, const char* s2){
  while(*s1 && (*s1==*s2))
    s1++,s2++;
  return *(const unsigned char*)s1-*(const unsigned char*)s2;
}

int strncmp2(const char* s1, const char* s2, long n ) {
  while(*s1 && (*s1==*s2)&&n>=0)
    s1++,s2++,n--;
  return *(const unsigned char*)s1-*(const unsigned char*)s2;
}

int atoi(char *str)
{
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-'){
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';

    return sign*res;
}


void exit() {
  int r=0;
  asm volatile( "svc #4    \n"
                "mov %0, r0\n"
              : "=r" (r) );
}

int unlink(const char *pathname) {
  int r;
  asm volatile( "mov r0, %1 \n"
                "svc #0x10    \n"
              : "=r" (r)
              : "r" (pathname)
              : "r0");
}

void ls() {
  int r=0;
  asm volatile( "svc #0x11    \n"
                "mov %0, r0\n"
              : "=r" (r) );
}

//0 for RW, 1 for R, 2 for W
int open(char* path, int oflag ) {
  int r;
  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #0x12     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (path), "r" (oflag)
              : "r0", "r1" );
  return r;
}


int cd(char* name) {
  int r=-1;
  asm volatile( "mov r0, %1 \n"
                "svc #0x13    \n"
                "mov %0, r0\n"
              : "=r" (r)
              : "r" (name)
              : "r0");
  return r;
}

int mkdir(char* name) {
  int r;
  if(strlen(name) == 0)
    return -1;
  asm volatile( "mov r0, %1 \n"
                "svc #0x14    \n"
                "mov %0, r0\n"
              : "=r" (r)
              : "r" (name)
              : "r0");
  return r;
}

void pwd(char* path) {
  int r;
  asm volatile( "mov r0, %1 \n"
                "svc #0x15    \n"
              : "=r" (r)
              : "r" (path)
              : "r0");
}

int lseek(int fd, int amount) {
  int r;
    asm volatile("mov r0, %1 \n"
               "mov r1, %2 \n"
               "svc #0x16    \n"
               "mov %0, r0\n"
              : "=r" (r)
              : "r" (fd), "r" (amount)
              : "r0");
  return r;
}

void write_no3(uint32_t n) {
    char s[10], temp[10];

    uint32_t x = n, i = 0;if(n == 0)  write(1, "0", 1);
    n = x;

    while(n!=0) {
      s[i] = '0' + n%10;
      n = n/10;
      i++;
    }
    for(int j = 0; j<i; j++)
      temp[j] = s[i-j-1];
    write(1, temp, i);
}

void write_str3(char* s) {
  int len = 0;
  char* scpy = s;
  while(*scpy) {
    len++;
    scpy++;
  }
  write(1, s, len);
}
