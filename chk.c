/* Copyright (c) 1998, 1999 Nickel City Software */

#include <stdio.h>

#define __CHK_C__
#include "chk.h"


void
chksetup(void)
{
  printf("This executable was compiled with:\n");
  printf("mchk version %2.2f %s\n%s\n", VERSION, ALPHABETA, COPYRIGHT);

  printf("Options: ");

#ifdef DO_STACK_TRACE
  printf(" STACKTRACE(");
# ifdef SOLARIS
  printf("SOLARIS");
# elif defined(LINUX)
  printf("LINUX");
# endif
  printf(")");
#endif

#ifdef DEMANGLE_GNU_CXX
  printf(" DEMANGLE(GNU_CXX)");
#endif

  printf("\n");

  printf("\n");
}

void
wchk(void *sp, void *ptr, size_t len)
{
  memnode *n;

  D(("write chk: ptr=0x%8.8X len=%-9d sp=0x%8.8X\n", ptr, len, sp));

#ifdef NO_STACK
  if(ptr >= sp) {
    /* we don't do checking of stack (automatic) variables  */
    return;
  }
#endif

  if(ptr == NULL) {
    NULLADDR;
    return;
  }

  LOCK;
  n = findMemnode(allocList, ptr);
  
  /* if (addr is not on the list of known allocated addresses) then
   *     warning: writing unallocated memory
   */
  
  if(!n) {
    n = findMemnode(freeList, ptr);
    /* if (addr is on the free list) then
     *     warning: writing free'd memory
     */
    if(n) {
      WRITEFREE(n);
    } else {
      /* BADADDR; */
      /* we'll assume that this is caused by looking up something on the
       * stack */
    }      
    UNLOCK;
    return;
  }
  
  n->state = STATE_DEF;
  
  /* if (addr is in the redzone of alloc'd memory) then
   *     warning: under/over-flow write
   * endif
   */
  
  /* 0 1 2 3 4 5 6 7 8 9 1 1
   *                     0 1
   * r r r r v v v v r r r r
   * 
   * ptr     ptr+4   ptr+size-4
   */

  if(ptr < (n->ptr + REDZONESIZE)) {
    UNDERWRITE(n);
  }
  else if(ptr >= (n->ptr + n->size - REDZONESIZE)) {
    OVERWRITE(n);
  }
  else if((ptr + len) > (n->ptr + n->size - REDZONESIZE)) {
    OVERWRITE(n);
  }
  
  UNLOCK;
}

void 
rchk(void *sp, void *ptr, size_t len)
{
  memnode *n;

  D(("read  chk: ptr=0x%8.8X len=%-9d sp=0x%8.8X\n", ptr, len, sp));

#ifdef NO_STACK
  if(ptr >= sp) {
    /* we don't do checking of stack (automatic) variables  */
    return;
  }
#endif

  if(ptr == NULL) {
    NULLADDR;
    return;
  }

  LOCK;
  n = findMemnode(allocList, ptr);
  
  /* if (addr is not on the list of known allocated addresses) then
   *     warning: reading unallocated memory
   */

  if(!n) {
    n = findMemnode(freeList, ptr);

    /* if (addr is on the free list) then
     *     warning: reading free'd memory
     */
    if(n) {
      READFREE(n);
    } else {
      /* BADADDR; */ 
      /* we'll assume that this is caused by looking up something on the
       * stack */
    }      
    UNLOCK;
    return;
  }
  
  /* if (addr is in the redzone of alloc'd memory) then
   *     warning: under/over-flow read
   */

  if(ptr < (n->ptr + REDZONESIZE)) {
    UNDERREAD(n);
  }
  else if(ptr >= (n->ptr + n->size - REDZONESIZE)) {
    OVERREAD(n);
  }
  else if((ptr + len) > (n->ptr + n->size - REDZONESIZE)) {
    OVERREAD(n);
  }

  /*if (addr was not previously written to) then
   *     warning: reading uninitialized memory
   */

  if(n->state == STATE_UNDEF) {
    BADREAD(n);
  }

  UNLOCK;
}

void
chkexit()
{
  unsigned int c  = 0;
  unsigned int lm = 0;
  memnode     *p;

  LOCK;
  for(p = allocList ; p ; p = p->next) {
    lm += (p->size - REDZONESIZE*2);
    c++;
  }
  UNLOCK;

  if(c) {
    printf("%u chunk%c of memory leaked (%u bytes)\n", 
	   c, 
	   (c > 1)?'s':0, 
	   lm);
  }

  if(staticOffset) {
    printf("%u bytes of staticMemory used while initializing.\n",
	   staticOffset);
  }
}



