/* Copyright (c) 1998, 1999 Nickel City Software */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>

#ifdef SOLARIS
# include <setjmp.h>
# include <sys/reg.h>
# include <sys/frame.h>
# if defined(sparc) || defined(__sparc)
#  define FLUSHWIN() asm("ta 3");
#  define FRAME_PTR_INDEX 1
#  define SKIP_FRAMES 0
# endif

# if defined(i386) || defined(__i386)
#  define FLUSHWIN() 
#  define FRAME_PTR_INDEX 3
#  define SKIP_FRAMES 1
# endif
#endif

#ifndef RTLD_NOW
# define RTLD_NOW 1
#endif

#define MALLOC_SYM "malloc"
#define FREE_SYM   "free"

#define __MCHK_CORE__
#include "chk.h"

#ifdef DEBUG
# define D(X) printf X
#else
# define D(X)
#endif

#ifdef THREAD_SAFE
# include <pthread.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
# define LOCK                                           \
{                                                       \
  int __ptmlret = pthread_mutex_lock(&lock);            \
  if(__ptmlret) {                                       \
     printf("chk.o: pthread_mutex_lock returned %d\n",  \
            __ptmlret);                                 \
     exit(-1);                                          \
  }                                                     \
}
# define UNLOCK                                          \
{                                                        \
  int __ptmulret = pthread_mutex_unlock(&lock);          \
  if(__ptmulret) {                                       \
     printf("chk.o: pthread_mutex_unlock returned %d\n", \
            __ptmulret);                                 \
     exit(-1);                                           \
  }                                                      \
}
#else
# define LOCK
# define UNLOCK
#endif

#define BADFREE    printf("BADFREE   ptr=%X\n", ptr); walkStack()
#define REFREE     printf("REFREE    ptr=%X\n", ptr); walkStack()
#define READFREE   printf("READFREE  ptr=%X\n", ptr); walkStack()
#define WRITEFREE  printf("WRITEFREE ptr=%X\n", ptr); walkStack()

#define BADADDR    printf("BADADDR   ptr=%X\n", ptr); walkStack()
#define BADWRITE   printf("BADWRITE  ptr=%X\n", ptr); walkStack()
#define BADREAD    printf("BADREAD   ptr=%X\n", ptr); walkStack()
#define UNDERWRITE printf("UNDERWRITE ptr=%X\n", ptr); walkStack()
#define OVERWRITE  printf("OVERWRITE ptr=%X\n", ptr); walkStack()
#define UNDERREAD  printf("UNDERREAD ptr=%X\n", ptr); walkStack()
#define OVERREAD   printf("OVERREAD  ptr=%X\n", ptr); walkStack()
#define NULLADDR   printf("NULLADDR  ptr=%X\n", ptr); walkStack()

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
      WRITEFREE;
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
    UNDERWRITE;
  }
  else if(ptr >= (n->ptr + n->size - REDZONESIZE)) {
    OVERWRITE;
  }
  else if((ptr + len) > (n->ptr + n->size - REDZONESIZE)) {
    OVERWRITE;
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
      READFREE;
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
    UNDERREAD;
  }
  else if(ptr >= (n->ptr + n->size - REDZONESIZE)) {
    OVERREAD;
  }
  else if((ptr + len) > (n->ptr + n->size - REDZONESIZE)) {
    OVERREAD;
  }

  /*if (addr was not previously written to) then
   *     warning: reading uninitialized memory
   */

  if(n->state == STATE_UNDEF) {
    BADREAD;
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
}

static int
addAlloc(size_t size, void *ptr)
{
  memnode *n = realmalloc(sizeof(memnode));
  memnode *p;

  if(!n) return -1;

  n->size  = size;
  n->ptr   = ptr;
  n->state = STATE_UNDEF;
  n->next  = NULL;
  n->prev  = NULL;
  n->freedTime = 0;

  LOCK;
  if(! allocList) {
    allocList = n;
  } else {
    for(p = allocList ; p->next ; p = p->next);
    n->prev = p;
    p->next = n;
  }
  UNLOCK;

  return 0;
}

static int
addFree(memnode *n)
{
  memnode        *p;
  struct timeval  tv;

  if(gettimeofday(&tv, NULL)) {
    perror("chk.o: gettimeofday");
  } else {
    n->freedTime = tv.tv_sec;
  }

  /* add the new node to the freeList */

  if(!freeList) {
    freeList = n;
  } else {
    for(p = freeList ; p->next ; p = p->next);
    p->next = n;
    n->prev = p;
  }

  /* free any nodes that are too old */

  for(p = freeList ; p ; p = p->next) {
    if(p->freedTime < (tv.tv_sec - freeAge)) {
      if(p = freeList) freeList = p->next;
      closeHole(p);
      freeMemnode(p);
    }
  }

  return 0;
}

static memnode *
findMemnode(memnode *list, void *ptr)
{
  memnode *p;
  int      done;

  if(!ptr) {
    D(("findMemnode: ptr == NULL\n"));
    return NULL;
  }

  done = 0;
  p    = list;

  /* we check to see if the ptr is WITHIN a known block of memory */

  while(!done) {
    if(!p) {
      D(("findMemnode: at end of list\n"));
      done = 1;
    } else {
      D(("findMemnode: examining memnode %X <= %X <= %X\n",
	 p->ptr, ptr, p->ptr + p->size));
      if((p->ptr <= ptr) && (ptr <= (p->ptr + p->size))) {
	D(("findMemnode: true.\n"));
	done = 1;
      } else {
	D(("findMemnode: false.\n"));
      }
    }
    if(!done) p = p->next;
  }

  if(!p) {  /* didnt find it */
    D(("findMemnode: didn't find anything.\n"));
    return NULL;
  }
  D(("findMemnode: found a match.\n"));
  return p;
}

static void
freeMemnode(memnode *p)
{
  if(p && p->ptr) {
    realfree(p->ptr);
    realfree(p);
  }
}

static void
closeHole(memnode *p) 
{
  if(!p) return;

  if(p->prev) {
    if(p->next) {
      p->prev->next = p->next;
      p->next->prev = p->prev;
    } else {
      p->prev->next = NULL;
    }
  } else {
    if(p->next) {
      p->next->prev = NULL;
    }
  }

  p->next = NULL;
  p->prev = NULL;
}

void *
malloc(size_t size)
{
  void *ptr = NULL;

  /*printf("malloc(%d)\n", size);*/

  if(malloc_init == 0) {
    LOCK;
    if(loadLibC(LIBC)) {
      exit(-1);
    }
    malloc_init = 1;
    UNLOCK;
  }

  if(size == 0) return (void *) NULL;
  size += REDZONESIZE * 2; 
  ptr = realmalloc(size);
  if(! ptr) return ptr;

  memset(ptr, 0, size);

  if(addAlloc(size, ptr) != 0) {
    realfree(ptr);
    return NULL;
  }

  return ptr + REDZONESIZE;
}

void
myfree(void *ptr)
{
  memnode *p;

  ptr -= REDZONESIZE; /* move to beginning of redzone */

  LOCK;
  for(p = allocList ; p && (p->ptr != ptr) ; p = p->next);
  if(p) {
    if(p == allocList) allocList = p->next;
    closeHole(p);
    addFree(p);
  } else {
    for(p = freeList ; p && (p->ptr != ptr) ; p = p->next);
    if(p) {
      REFREE;
    } else {
      BADFREE;
    }
  }
  UNLOCK;
  free(ptr);
}

/* the following concept (dyn loading libc instead of re-writting
 * malloc) was borrowed from ccmalloc.
 */

static int
loadLibC(char *libc)
{
  void *handle;

  handle = dlopen(libc, RTLD_NOW);
  if(!handle) {
    perror("dlopen");
    return -1;
  }

  realmalloc = (void*(*)(size_t)) dlsym(handle, MALLOC_SYM);
  if(!realmalloc) {
    perror("dlsym[malloc]");
    dlclose(handle);
    return -1;
  }

  realfree = (void(*)(void*)) dlsym(handle, FREE_SYM);
  if(!realfree) {
    perror("dlsym[free]");
    dlclose(handle);
    return -1;
  }

  return 0;
}

/* the following code was adapted from a SUN example source file
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 *
 * #pragma ident   "@(#)print_my_stack.c   1.3     97/04/28 SMI"
 */

static int
walkStack(void)
{
#ifdef SOLARIS
  struct frame *sp;
  jmp_buf env;
  int i;
  
  FLUSHWIN();
  (void) setjmp(env);
  sp = (struct frame *) env[FRAME_PTR_INDEX];
  
  for(i=0;i<SKIP_FRAMES && sp;i++)
    sp = (struct frame *)sp->fr_savfp;
  
  while(sp && sp->fr_savpc) {
    printAddr((void*)(sp->fr_savpc));
    sp = (struct frame *)sp->fr_savfp;
  }
  
#endif

  return(0);
}

static void 
printAddr(void *pc)
{
#ifdef SOLARIS
  Dl_info info;
  
  if(dladdr(pc, & info) == 0) {
    (void) printf("\t<unknown>: 0x%x\n", (int)pc);
    return;
  }
  
  /*
   * filter out any mention of rtld.  We're hiding the cost
   * of the rtld_auditing here.
   */
  if (strstr(info.dli_fname, "ld.so.1"))
    return;
  
  (void) printf("\t%s:%s+0x%x\n", 
		info.dli_fname,
		info.dli_sname,
		(unsigned int)pc - (unsigned int)info.dli_saddr);
#endif
}
