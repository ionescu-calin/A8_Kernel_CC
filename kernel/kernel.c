#include "kernel.h"
#include <string.h>
/* Since we *know* there will be 2 processes, stemming from the 2 user
 * programs, we can
 *
 * - allocate a fixed-size process table (of PCBs), and use a pointer
 *   to keep track of which entry is currently executing, and
 * - employ a fixed-case of round-robin scheduling: no more processes
 *   can be created, and neither is able to complete.
 */

//writes a number, function used for debugging
void write_no(uint32_t n) {
    char s[20], temp[20];
    uint32_t x = n, i = 0;if(n == 0)  write(1, "0", 1);
    n = x;
    while(n!=0) {
      s[i] = '0' + n%10;
      n = n/10;
      i++;
    }
    for(int j = 0; j<i; j++)
    	PL011_putc( UART0, (s[i-j-1]) );
      PL011_putc( UART0, ('\n') );

  }
//writes a string, function used for debugging
void write_str(char* x) {
  while(*x!='\0')
    PL011_putc( UART0, *x++ );
}

pcb_t pcb[ maxProcesses ], *current = NULL;
oft_t oft;
int noProcesses = 1;

char buffer[100];                //STDIN buffer
int bufferIndex = 0;             //set the STDIN buffer index

Stack activeStack[maxProcs + 1]; //Convention: stack starts from 1 (it is easier to pop elements)
uint32_t activeBos = 0;          //Bottom of stack 0 when the stack is empty


//-------------------------------------START OF STACK RELATED FUNCTIONS--------------------------------------//
/* 1 if successful
 * 0 if the process already has a smaller priority
 * -1 if the process does not exist on the stack
 */
int decreasePriority(uint32_t pid, uint32_t priority) {
  pcb_t* proc = &pcb[ pid ];
  uint32_t *currentPriority = &activeStack[ proc->stackLocation ].priority;
  if( proc->status != ACTIVE)
    return -1;
  else
    if( *currentPriority < priority)
      return 0;
    else {
      *currentPriority = priority;
      while((*currentPriority < activeStack[ proc->stackLocation/2 ].priority)&&proc->stackLocation>1) {
        Stack aux = activeStack[ proc->stackLocation ];
        //update location for the parent
        pcb[ activeStack[ proc-> stackLocation/2 ].pid ].stackLocation*=2;
        pcb[ activeStack[ proc-> stackLocation/2 ].pid ].stackLocation+=(proc->stackLocation)%2;

        activeStack[ proc->stackLocation ] = activeStack[ proc->stackLocation/2 ];
        activeStack[ proc->stackLocation/2 ] = aux;
        //swap priority values
        currentPriority = &activeStack[ proc-> stackLocation/2].priority ;
        //proc becomes the new parent
        proc->stackLocation /=2;
      }
    }
  return 1;
}


/* Returns:
 *-1 if process already is on the stack (ACTIVE)
 * 1 otherwise  and makes process active + inserts it
 */
int insert(uint32_t pid, uint32_t priority) {
  //pcb[pid].priority = priority;
  if(pcb[ pid ].status == ACTIVE)
    return -1;
  else {
    pcb[ pid ].status = ACTIVE;
    activeBos++;
    pcb[ pid ].stackLocation = activeBos;
    activeStack[activeBos].priority = lowestPriority;
    activeStack[activeBos].pid = pid;
    return decreasePriority(pid, priority);
  }
  return 1;
}

/* Returns the pid of the process with the lowest priority
 * -1 if there is no active process
 * -2 if the stack is empty
 */
uint32_t pop_min() {
  int pid = activeStack[1].pid;
  if(activeBos <= 0)
    return -2;
  if(pcb[ pid ].status == ACTIVE) {
    if(activeBos > 0) {
      activeStack[1] = activeStack[activeBos];
      int i = 1;
      while((activeStack[i].priority > activeStack[i*2].priority || activeStack[i].priority > activeStack[i*2+1].priority)&&i<=activeBos) {
        int diff1 = activeStack[i].priority - activeStack[i*2].priority;
        int diff2 = activeStack[i].priority - activeStack[i*2 + 1].priority;
        Stack aux = activeStack[i];
        if(diff1 >= diff2) {
          activeStack[i] = activeStack[i*2];
          activeStack[i*2] = aux;
          pcb [ activeStack[i].pid ].stackLocation*=2;
          pcb [ activeStack[i*2].pid ].stackLocation/=2;
          i = i*2;
        }
        else {
          activeStack[i] = activeStack[i*2+1];
          activeStack[i*2+1] = aux;
          pcb [ activeStack[i].pid ].stackLocation*=2;
          pcb [ activeStack[i].pid ].stackLocation+=1;
          pcb [ activeStack[i*2+1].pid ].stackLocation/=2;
          i = i*2 + 1;
        }
      }
    }
    pcb[pid].status = SCHEDULED;
    activeStack[activeBos].priority = lowestPriority; //reset priority
    activeBos--;
    return pid;
  }
  else
    return -1;
}

void initialiseStack() {
  for(int i=1; i<maxProcs+1; i++) {
    activeStack[i].pid = -1;
    activeStack[i].priority = lowestPriority;
  }
}

//Computes the a new priority for a process just before it is replaced
//Applies aging. The priority of the current process remains unchanged, while
//the priority of the other processes increases by 1
uint32_t newPriority(uint32_t pid) {
  for(int i = 1; i <= activeBos; i++) {
    if(activeStack[i].pid != pid)
      activeStack[i].priority --;
  }
}
//-------------------------------------END OF STACK RELATED FUNCTIONS--------------------------------------//

//--------------------------------------IPC----------------------------------------------------------------//
//Erases the buffer of a pipe
void emptyPipeBuffer(pipe_t* pipe) {
  for(int i =0; i< pipeBufferSize; i++)
    pipe->buffer[i] = 0;
  pipe->bufferIndex = 0;
}

//returns -1 if not found, or entry in oft
int find_oft_entry_pipe(pipe_t* pipe) {
  for(int i = 0; i< pipeNo; i++)
    if(&oft.pipe[i] == pipe)
      return i;
  return -1;
}

//returns -1 if not found, or entry in oft
int find_oft_entry_file(file_t* file) {
  for(int i = 0; i< fileNo; i++)
    if(&oft.file[i] == file)
      return i;
  return -1;
}

//finds an empty space in the oft for a pipe
int find_oft_free_pipe() {
  for(int i = 0; i < fileNo; i++)
    if(oft.used_pipe[i] == 0)
      return i;
  return -1;
}

//finds an empty space in the oft for a file
int find_oft_free_file() {
  for(int i = 0; i < fileNo; i++)
    if(oft.used_file[i] == 0)
      return i;
  return -1;
}

/*
* -1 fail, 1 success, 0 already closed
* if fd is the last file descriptor, the resources associated with the open file are freed;
* closes file descriptor
*/
int close_kernel(int fd) {
  if(fd < 0 || fd > fdNo)  //out of bounds error checking
    return -1;
  else if(current->fdt[fd].active != 0)
    current->fdt[fd].active = 0;
  else return 0;
  if(current->fdt[fd].type == PIPE) {
    if(current->fdt[fd].pipep->fdCount == 1) {
      emptyPipeBuffer(current->fdt[fd].pipep);
      oft.used_pipe[find_oft_entry_pipe(current->fdt[fd].pipep)] = 0;
      oft.pipeCount --;
    }
    current->fdt[fd].pipep->fdCount--;
  } else if(current->fdt[fd].type == FILE) {
      oft.used_pipe[find_oft_entry_file(current->fdt[fd].filep)] = 0;
      oft.fileCount --;
  }
  return 1;
}

//creates a new fd and returns its no (selects the first available fd place)
int openfd() {
  for(int i = 0; i < fdNo; i++)
    if(current->fdt[i].active == 0) {
      current->fdt[i].active = 1;
      return i;
    }
  return -1;
}

/*
* return 0 for success, -1 for fail (POSIX)
* Creates the two fds
* pipefd[0] - read; pipefd[1] - write
*/
int pipe_kernel (int* pipefd) {
  int i = 0, fdtFound = 0;
  while(i < fdNo && fdtFound != 2) {
    if(current->fdt[i].active == 0) {
      pipefd[fdtFound] = i;
      current->fdt[i].active = 1;
      current->fdt[i].type = PIPE;
      if(fdtFound == 0)
        current->fdt[i].perm = READ;
      else
        current->fdt[i].perm = WRITE;
      fdtFound++;
    }
    i++;
  }

  if( fdtFound != 2) {
    if(fdtFound == 1)
      close_kernel(pipefd[0]);
    return -1;
  }
  else {
    int place = find_oft_free_pipe();
    if(place == -1) return -1;

    oft.used_pipe[place]  = 1;
    current->fdt[pipefd[0]].pipep = &oft.pipe[place];
    current->fdt[pipefd[1]].pipep = &oft.pipe[place];
    oft.pipe[place].active  = 1;
    oft.pipe[place].fdCount = 2;
    oft.pipeCount++;

    return 0;
  }
}

//replaces the oldfd of the current process with newfd if newfd already exists else return 0 or -1 if fd out of bounds
//also closes newfd
int dup2kernel(int oldfd, int newfd){
  if(oldfd == newfd) //Nothing happens if they are the same.
    return 1;
  if(oldfd < 0 || oldfd > fdNo || newfd < 0 || newfd >fdNo) //error checking
    return -1;
  if(current->fdt[newfd].active == 1) {
    current->fdt[oldfd] = current ->fdt[newfd];
    current->fdt[oldfd].pipep-> fdCount +=2;
    close_kernel(newfd);
    return 1;
  }
  else
    return 0;
}

//Sets up STDIN, STDOUT and sets the other files/pipes to null
void initialisefdt(int pid) {
  for(int i = 0; i < fdNo; i++) {
    pcb[pid].fdt[i].fp = 0;
    if(i == 0) {
      pcb[pid].fdt[i].active = 1;
      pcb[pid].fdt[i].perm = READ;
      pcb[pid].fdt[i].type = STDI;
    }
    else if (i == 1) {
      pcb[pid].fdt[i].active = 1;
      pcb[pid].fdt[i].perm = WRITE;
      pcb[pid].fdt[i].type = STDO;
    }
    else
      pcb[pid].fdt[i].active = 0;
    pcb[pid].fdt[i].filep = NULL;
    pcb[pid].fdt[i].pipep = NULL;
  }
}

void initialise_pcb(pcb_t pcb[100]) {
  //uint32_t entry_P0, entry_P1, entry_P2;  //will get replaced by shell
  //asm volatile( "LDR %0, =P3 \n" :"=r"(entry_P3));
  for(int i = 0; i<100; i++) {
    strcpy(pcb[i].path, "");
    pcb[i].pid = -1;
    pcb[i].status = DEAD;
    initialisefdt(i);
  }
}

//sets all pipes to inactive and resets the pipe buffer
void initialise_pipes() {
  for(int i = 0; i<pipeNo; i++) {
    oft.pipe[i].active = 0;
    oft.pipe[i].fdCount = 0;
    for(int j = 0; j<pipeBufferSize; j++)
      oft.pipe[i].buffer[j] = 0;
  }
}
void initialise_oft() {
  oft.fileCount = 0;
  oft.pipeCount = 0;
  for(int i = 0; i < fileNo; i++)
    oft.used_file[i] = 0;
  for(int i = 0; i < pipeNo; i++)
    oft.used_pipe[i] = 0;
  initialise_pipes();
}

void startInitialise() {
  initialise_pcb(pcb);
  initialise_oft();
  initialiseStack();
}

//the basic round robin scheduler
void schedulerRR( ctx_t* ctx ) {
	int pid = current -> pid + 1;
	int found = 0;
	while(pid < noProcesses && !found)
		if(pcb[pid].pid!=-1)
			found = 1;
		else pid++;
	if(!found) pid = 0;
	while(!found){
		if(pcb[pid].pid!=-1)
			found = 1;
		else pid++;
	}
	memcpy( &pcb[ current->pid ].ctx, ctx, sizeof( ctx_t ) );
	memcpy( ctx, &pcb[ pid ].ctx, sizeof( ctx_t ) );
	current = &pcb[ pid ];
}

//Simmilar to sjf. It uses priorities and aging that is computed by the
//newPrio function
void scheduler2( ctx_t* ctx) {
    int newPrio = newPriority(current->pid);
    //write_no(current->priority);
    if(newPrio < 0) {
      newPrio = current -> priority;
    }
    if(pcb[current->pid].status == SCHEDULED) {
      insert(current->pid, current->priority);
    }
    uint32_t pid = pop_min();
    memcpy( &pcb[ current->pid ].ctx, ctx, sizeof( ctx_t ) );
    memcpy( ctx, &pcb[ pid ].ctx, sizeof( ctx_t ) );
    current = &pcb[ pid ];

}
//intermediate function to select between schedulers
void scheduler( ctx_t* ctx ) {
	if(scheduler_type == 0)
		schedulerRR(ctx);
	else if(scheduler_type == 2)
		scheduler2(ctx);
	else if(scheduler_type == 3);
}


void intialise_pcb(pcb_t pcb[100]) {
  for(int i = 0; i<100; i++)
    pcb[i].pid = -1;
}

//changes the program with some other program specified by the name
//and assigns it a new priority. if priority == -1, the default one will be used
void execpn( ctx_t* ctx, char* name, int priority) {
  current->ctx.cpsr = 0x50;
  current->ctx.sp   = current->tos;

  if      (!strcmp(name, "P0")) ctx->pc = ( uint32_t )entry_P0;
  else if (!strcmp(name, "P1")) ctx->pc = ( uint32_t )entry_P1;
  else if (!strcmp(name, "P2")) ctx->pc = ( uint32_t )entry_P2;
  else if (!strcmp(name, "P3")) ctx->pc = ( uint32_t )entry_P3;
  else if (!strcmp(name, "T0")) ctx->pc = ( uint32_t )entry_T0;
  else if (!strcmp(name, "T1")) ctx->pc = ( uint32_t )entry_T1;
  else if (!strcmp(name, "T2")) ctx->pc = ( uint32_t )entry_T2;
  else if (!strcmp(name, "T3")) ctx->pc = ( uint32_t )entry_T3;
  else if (!strcmp(name, "shell")) ctx->pc = ( uint32_t )entry_shell;
  if(priority == -1){
    current->priority = def_priority;
  }
  else{
    current->priority = priority;
  }

}

//exits a program
void exit_kernel() {
  memset( current, 0, sizeof( pcb_t ) );
  //Children are inherited by the parent of the process that's being killed
  /* if(current != &pcb[0])
  for(int i = 1; i<100; i++)
    if(pcb[ i ].ppid == current->pid)
      pcb[ i ].ppid = pcb [ current -> ppid ].pid;
  */
  for(int i = 0; i<=fdNo;i++)
    current->fdt[i].active = -1;
  current->pid = -1;
  current->status = DEAD;
  initialisefdt(current->pid);
  noProcesses--;
}


void kernel_handler_rst( ctx_t* ctx              ) {
  /* Initialise PCBs representing processes stemming from execution of
   * the two user programs.  Notentry_P1e in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR
   *   mode, with IRQ interrupts enabled, and
   * - the PC and SP values matche the entry point and top of stack.
   */

  //Sets up UART
  UART0->IMSC           |= 0x00000010; // enable UART    (Rx) interrupt
  UART0->CR              = 0x00000301; // enable UART (Tx+Rx)

  //Sets up timer to generate IRQ
  TIMER0->Timer1Load     = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl     = 0x00000042; // select 32-bit, periodic timer
  TIMER0->Timer1Ctrl    |= 0x000000A0; // enable timer, with interrupts


  GICC0->PMR             = 0x000000F0; // unmask all          interrupts
  GICD0->ISENABLER[ 1 ] |= 0x00001010; // enable UART    (Rx) interrupt
  GICC0->CTLR            = 0x00000001; // enable GIC interface
  GICD0->CTLR            = 0x00000001; // enable GIC distributor

  //for stage 0
  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
  memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );
  memset( &pcb[ 2 ], 0, sizeof( pcb_t ) );
  //initialise pcbs, oft, pipes and file system
  startInitialise();
  create_root();

  if(stage == 1) {
    pcb[ 0 ].pid      = 0;
    pcb[ 0 ].ctx.cpsr = 0x50;
    pcb[ 0 ].ctx.pc   = ( uint32_t )( entry_shell );
    pcb[ 0 ].ctx.sp   = ( uint32_t )(  &tos_P0 );
    pcb[ 0 ].tos = ( uint32_t ) &tos_P0;
    pcb[ 0 ].status = DEAD;
    /* Once the PCBs are initialised, we (arbitrarily) select one to be
     * restored (i.e., executed) when the function then returns.
     */

    current = &pcb[ 0 ]; memcpy( ctx, &current->ctx, sizeof( ctx_t ) );
    pcb[0].priority = 5;
    insert(0,5);
  }
  else {
    pcb[ 0 ].pid      = 0;
    pcb[ 0 ].ctx.cpsr = 0x50;
    pcb[ 0 ].ctx.pc   = ( uint32_t )( entry_P0 );
    pcb[ 0 ].ctx.sp   = ( uint32_t )(  &tos_P0 );
    pcb[ 0 ].tos = ( uint32_t ) &tos_P0;
    pcb[ 0 ].status = DEAD;

    pcb[ 1 ].pid      = 1;
    pcb[ 1 ].ctx.cpsr = 0x50;
    pcb[ 1 ].ctx.pc   = ( uint32_t )( entry_P1 );
    pcb[ 1 ].ctx.sp   = ( uint32_t )(  &tos_P1 );
    pcb[ 1 ].tos = ( uint32_t ) &tos_P1;
    pcb[ 1 ].status = DEAD;

    pcb[ 2 ].pid      = 2;
    pcb[ 2 ].ctx.cpsr = 0x50;
    pcb[ 2 ].ctx.pc   = ( uint32_t )( entry_P2 );
    pcb[ 2 ].ctx.sp   = ( uint32_t )(  &tos_P2 );
    pcb[ 2 ].tos = ( uint32_t ) &tos_P2;
    pcb[ 2 ].status = DEAD;
    noProcesses = 3;
    current = &pcb[ 0 ]; memcpy( ctx, &current->ctx, sizeof( ctx_t ) );
  }

  irq_enable();
  //DEBUGGING
  //create_metadata("hello2", 0);
  //file_t meta = read_file_meta(4100);
  //write_no(find_meta("hello", 0));
  //write_no(write_to_file(4096, 0, "NIVEL", strlen("NIVEL")));
  //char x[100], y[100];
  //int root = 4096;
  //write_no(create_file_dir_addr(root, "cats.txt", 0));
  //find_all_files_by_addr(find_file("folderTry/Folder", 1));
  //write_to_file(find_file("folder/cats.txt", 0), 0, "NIVEL", strlen("NIVEL"));
  //find_all_files_by_addr(root);
  //write_no(read_from_file(4108, 0, 5, x));
  //write_no(read_from_file(4108, 5, 5, x));
  //write_no(read_from_file(4108, 5, 5, y));
  //write_str(x);
  //write_str(y);
  //write_to_file(4096, 2, "rad");
  //delete_file("hello");
  //write_to_file(4097, 0, "lalalla");
  //write_no(create_file_part());
  //DEBUGGING
  return;
}

//Copy file descriptor from parent to child
void copy_parents_fd(uint32_t pid) {
  for(int i = 0; i< fdNo; i++)
    pcb[pid].fdt[i] = pcb[pcb[pid].ppid].fdt[i];
}

void kernel_handler_svc( ctx_t* ctx, uint32_t id ) {
  //irq_unable();
  /* Based on the identified encoded as an immediate operand in the
   * instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call,
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case 0x00 : { // yield()
      scheduler( ctx );
      PL011_putc( UART0, '>' );
      break;
    }
    case 0x01 : { // write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      fd_t currentfd = (*current).fdt[fd];
      if(currentfd.type == STDO) {
	    for( int i = 0; i < n; i++ ) {
	        PL011_putc( UART0, *x++ );
	      }
	      ctx->gpr[ 0 ] = n;
  	  }
  	  else if(currentfd.type == PIPE && currentfd.active == 1) {
	  	pipe_t* currentPipe = currentfd.pipep;
	  	for( int i = 0; i < n; i++ ) {
	  		currentPipe->buffer[i] = *x;
	        x+= sizeof(char);
	      }
	      currentPipe->bufferIndex+=n;
	      ctx->gpr[ 0 ] = n;
	  }
    else if(currentfd.type == FILE && currentfd.active == 1) {
      file_t* currentFile = currentfd.filep;
      //returns new fp
      if(currentfd.filep->permissions == 1)
        ctx->gpr[ 0 ] = -1;
      else {
        current->fdt[fd].fp = write_to_file(currentfd.file_addr, currentfd.fp, x, n);
        ctx->gpr[ 0 ] = current->fdt[fd].fp;
      }
    }
      break;
    }
	case 0x02 : { //read(fd, x, n)
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      fd_t currentfd = (*current).fdt[fd];
      //if(currentfd.active == 0); //we have a problem;
      if(currentfd.type == STDI) {
      	if(bufferIndex != 0) {
	      	int count = 0;
	      	if (bufferIndex -1 < n)
	      		count = bufferIndex;
	      	else
	      		count = n;

	        for(int i = 0; i<count; i++)  {
	          	*x = buffer[i];
	          	x+= sizeof(char);
	        }

          //Gets the input from the buffer and deletes the characters that it read, setting the buffer index
          //to the appropriate position
	        for(int i = 0; i < bufferIndex - count; i++)
	        		buffer[i] = buffer[count + i];
	        ctx->gpr[ 0 ] = count;
	        bufferIndex -= count;
	    }
	    else ctx->gpr[ 0 ] = 0;
      }
      //Reading from a PIPE
      else if(currentfd.type == PIPE && currentfd.active == 1) {
      	pipe_t* currentPipe = currentfd.pipep;
      	int pipeBufferIndex = currentPipe->bufferIndex;

      	if(pipeBufferIndex != 0) {
	      	int count = 0;
	      	if (pipeBufferIndex -1 < n)
	      		count = pipeBufferIndex;
	      	else
	      		count = n;

	        for(int i = 0; i<count; i++)  {
	          	*x = currentPipe->buffer[i];
	          	x+= sizeof(char);
	        }
	        if(pipeBufferIndex>count)
	        	for(int i = pipeBufferIndex - count; i < pipeBufferIndex; i++)
	        		 currentPipe->buffer[i] =  currentPipe->buffer[count + i];
	        ctx->gpr[ 0 ] = pipeBufferIndex;
	        currentPipe->bufferIndex -= count;
	    }
	    else ctx->gpr[ 0 ] = 0;
	  }
    //Reading from a file also returns the new file pointer
    //retunrs -1 if permissions are wrong
    else if(currentfd.type == FILE && currentfd.active == 1) {
      if(currentfd.filep->permissions == 2)
        ctx->gpr[ 0 ] = -1;
      else {
        file_t* currentFile = currentfd.filep;
        //returns new fp
        current->fdt[fd].fp = read_from_file(currentfd.file_addr, current->fdt[fd].fp, n, x);
        ctx->gpr[ 0 ] = current->fdt[fd].fp;
      }
    }

      break;
    }
    case 0x03: { //fork()
        //creates an identical copy of the calling process, with a different pid
        //find a place for the process in the pcb
        int processPlace = 0;
        while((pcb[ processPlace ].pid) != -1)
          processPlace ++;
  		  if(processPlace>maxProcesses) {
  			 ctx->gpr[0] = -1;
  		  }
  		  else {
         memcpy( &pcb[ processPlace ].ctx, ctx, sizeof( ctx_t ) );

         pcb[ processPlace ].ppid     = current->pid;   //child parent ID
         pcb[ processPlace ].pid      = processPlace;

         uint32_t offset = current->tos - ctx->sp ;
         //assign top of stack and stack pointer for the new process
         pcb[ processPlace ].tos      = (uint32_t) (pcb[ 0 ].tos + processPlace*0x00001000);
         pcb[ processPlace ].ctx.sp   = (uint32_t) (pcb[ processPlace ].tos - offset);

         //copies the stack
         memcpy( (uint32_t*) (pcb[processPlace].tos- offset), (uint32_t*) (current->tos - offset), offset);
         //process inherts fds of the partent
         copy_parents_fd(processPlace);
         ctx->gpr[0] = processPlace;
         pcb[ processPlace ].ctx.gpr[0] = 0;
        //set the priority of the process to be the same as the current one so it will be scheduled next
         pcb[ processPlace ].priority = current->priority;
         insert(processPlace, current->priority);
         noProcesses++;

  		}
      break;
    }
    case 0x04: {  //exit()
      //resets the process pcb so it can be reused and removes pipes
      exit_kernel();
      //schedules the next process
      scheduler( ctx );
      break;
    }
    case 0x05: {  //execpn(char* name)
      //Execute a process with the default priority
      char* name = ( char* ) ctx->gpr[ 0 ];
      execpn(ctx, name, -1);
      break;
    }
    case 0x06: {  //pipe(int* pipefd)
      //Open a pipe chanel and return the two file descriptors
      int* pipefd = ( int*  )( ctx->gpr[ 0 ] );
      int r = pipe_kernel(pipefd);
      ctx->gpr[ 0 ] = r;
      break;
    }
    case 0x07: {  //dup2(int oldfd, in newfd)
      int oldfd = ( int   )( ctx->gpr[ 0 ] );
      int newfd = ( int   )( ctx->gpr[ 1 ] );
      int r = dup2kernel(oldfd, newfd);
      ctx->gpr[ 0 ] = r;
      break;
    }
    case 0x08: { //close (int fd)
      int fd = ( int   )( ctx->gpr[ 0 ] );
      int r = close_kernel(fd);
      ctx->gpr[ 0 ] = r;
      break;
    }
    case 0x09: { //execpnp(char* name, int priority)
      char* name = ( char* ) ctx->gpr[ 0 ];
      int   priority = ( int   )( ctx->gpr[ 1 ] );
      execpn(ctx, name, priority);
      break;
    }
    case 0x10: { //unlink(char* path)
      char* path = ( char* ) ctx->gpr[ 0 ];
      char aux_path[max_path_length];
      strcpy(aux_path, pcb[current->pid].path);
      //add name to the current path
      if(!strcmp(aux_path, ""))
        strcat(aux_path, path);
      else {
        strcat(aux_path, "/");
        strcat(aux_path, path);
      }
      //check to see if dir actualy exists, raise an error if not
      uint16_t addr = find_file(aux_path, 0);
      if(addr == error_place)
        addr = find_file(aux_path, 1);

      if(addr == error_place)
          ctx->gpr[ 0 ] = -1;
      else{
        //remove the links to that file and mark its blocks as unused
        strcpy(aux_path, pcb[current->pid].path);
        uint16_t current_addr = find_file(aux_path, 1);
        delete_from_dir(current_addr, addr);
        delete_file(addr);
        ctx->gpr[ 0 ] = 0;
      }

      break;
    }
    case 0x11: { //ls()
      //display the files in the current directory
      char aux_path[max_path_length];
      strcpy(aux_path, pcb[current->pid].path);
      find_all_files_by_addr(find_file(aux_path, 1));
      break;
    }
    case 0x12: { //open/creat(char* path, int oflag)
      char* name = ( char* ) ctx->gpr[ 0 ];
      int oflag = ctx->gpr[ 1 ];
      char aux_name [max_path_length];
      strcpy(aux_name, pcb[current->pid].path);
      //add file name to the current path
      if(!strcmp(aux_name, ""))
        strcat(aux_name, name);
      else {
        strcat(aux_name, "/");
        strcat(aux_name, name);
      }
      //create a copy of the path because find_file destroys it
      char path_cpy[max_path_length];
      strcpy(path_cpy, pcb[current->pid].path);

      uint16_t addr = find_file(aux_name, 0);
      //check to see if file exists, if not, then create
      if(addr == error_place) {
        addr = create_file_dir_addr(find_file(path_cpy, 0), name, 0, oflag);
      }
      //try to create a file descriptor, return -1 if it fails
      int fd = openfd();
      if(fd == -1)
        ctx->gpr[ 0 ] = -1;
      else {
        //look through oft to see if file is already open
        int i = 0, found = 0;
        while (i < fileNo && !found) {
          if(!strcmp(oft.file[i].name, name)) {
            current->fdt[fd].filep = &oft.file[i];
            found = 1;
          }
          i++;
        }

        file_t file;
        if( found == 0 ) {
        //if the file is not opened then make an entry in the oft
          file = read_file_meta(addr);
          oft.file[oft.fileCount] = file;
          current->fdt[fd].filep = &oft.file[oft.fileCount];
          oft.fileCount ++;
        }
        //check for permissions, return -2 if the check fails
        if(current->fdt[fd].filep->permissions != 0 && current->fdt[fd].filep->permissions != oflag) {
          close_kernel(fd);
          ctx->gpr[ 0 ] = -2;
        }
        else {
          current->fdt[fd].file_addr = addr;
          current->fdt[fd].active = 1;
          current->fdt[fd].type = FILE;
          current->fdt[fd].fp = 0;
          ctx->gpr[ 0 ] = fd;
        }
      }
      break;
    }
    case 0x13: { //cd(char* folder)
      char* name = ( char* ) ctx->gpr[ 0 ];
      char aux_name[max_path_length];
      char name_copy[max_path_length];

      if(!strcmp(name, "..")) {
        if(!strcmp(pcb[current->pid].path, "")) {
          ctx->gpr[ 0 ] = -1;
        }
        else {
          //remove the last directory from the current path if cd ..
          char* p = strrchr(pcb[current->pid].path, '/');
          if(p == NULL) {
            pcb[current->pid].path[0] = '\0';
          }
          else {
          *p = '\0';
          }
        }
        ctx->gpr[ 0 ] = 1;
      }
      else {  //if not cd .., add name of directory to the current path
        strcpy(aux_name, pcb[current->pid].path);
        if(!strcmp(aux_name, ""))
          strcat(aux_name, name);
        else {
          strcat(aux_name, "/");
          strcat(aux_name, name);
        }
        //search for the directory, if it does not exist, return -1
        //else change current path to contain the directory
        strcpy(name_copy, aux_name);
        if(find_file(aux_name, 1) != error_place) {
          strcpy(pcb[current->pid].path, name_copy);
          ctx->gpr[ 0 ] = 1;
        }
        else {
          ctx->gpr[ 0 ] = -1;
        }
    }
      break;
    }
    case 0x14: { //mkdir(char* name)
      char* name = ( char* ) ctx->gpr[ 0 ];
      char aux_path[max_path_length];
      strcpy(aux_path, pcb[current->pid].path);
      if(!strcmp(aux_path, ""))
        strcat(aux_path, name);
      else {
        strcat(aux_path, "/");
        strcat(aux_path, name);
      }
      uint16_t file_found = find_file(aux_path, 0);
      strcpy(aux_path, pcb[current->pid].path);
      uint16_t address = find_file(aux_path, 1);
      if(file_found == error_place) {
        create_file_dir_addr(address, name, 1, 0);
        ctx->gpr[ 0 ] = 1;
      }
      else
        ctx->gpr[ 0 ] = -1;
      break;
    }
    case 0x15: { //pwd()
      char* name = ( char* ) ctx->gpr[ 0 ];
      strcpy(name, pcb[current->pid].path);
      break;
    }
    case 0x16: { //lseek(int fd, int offset)
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      int  offset = ( int )( ctx->gpr[ 1 ] );

      current->fdt[fd].fp += offset;
      ctx->gpr[ 0 ] =  current->fdt[fd].fp;

      break;
    }
    case 0x17: {//execpnp(char* name, int priority)
      char* name = ( char* ) ctx->gpr[ 0 ];
      int   priority = ( int   )( ctx->gpr[ 1 ] );
      execpn(ctx, name, priority);
      break;
    }
    default   : { // unknown
      break;
    }
  }
  //irq_enable();
  return;
}

void kernel_handler_irq(ctx_t* ctx) {
  //irq_unable(); //So intrerrupts won't be handled when already handling an intrerrupt
  // Step 2: read  the interrupt identifier so we know the source.
  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_UART0 ) {
    buffer[bufferIndex] = PL011_getc( UART0 );
    bufferIndex++;
    UART0->ICR = 0x10;
  }
  else if( id == GIC_SOURCE_TIMER0 ) {
    TIMER0->Timer1IntClr = 0x01;
    scheduler(ctx);
  }
  // Step 5: write the interrupt identifier to signal we're done.
  GICC0->EOIR = id;
  //irq_enable();
}
