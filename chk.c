/* Copyright (c) 1998, 1999 Nickel City Software */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

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

#ifndef FALSE
# define FALSE 0
# define TRUE !FALSE
#endif

#define PRINTSTACK walkStack(printAddr, FALSE)
#define STORESTACK walkStack(storeAddr, TRUE)
#define FULLTRACE(X) \
{                                       \
  printf("allocated from:\n");          \
  printStacktrace(X->allocatedFrom);    \
  if(X->freedFrom) {                    \
    printf("freed from:\n");            \
    printStacktrace(X->freedFrom);      \
  }                                     \
}

#define BADFREE    printf("BADFREE   ptr=%X\n", ptr); PRINTSTACK
#define REFREE     printf("REFREE    ptr=%X\n", ptr); PRINTSTACK
#define READFREE   printf("READFREE  ptr=%X\n", ptr); PRINTSTACK
#define WRITEFREE(X) printf("WRITEFREE ptr=%X\n", ptr); FULLTRACE(X)
#define BADADDR    printf("BADADDR   ptr=%X\n", ptr); PRINTSTACK
#define BADWRITE   printf("BADWRITE  ptr=%X\n", ptr); PRINTSTACK
#define BADREAD    printf("BADREAD   ptr=%X\n", ptr); PRINTSTACK
#define UNDERWRITE printf("UNDERWRITE ptr=%X\n", ptr);PRINTSTACK
#define OVERWRITE  printf("OVERWRITE ptr=%X\n", ptr); PRINTSTACK
#define UNDERREAD  printf("UNDERREAD ptr=%X\n", ptr); PRINTSTACK
#define OVERREAD   printf("OVERREAD  ptr=%X\n", ptr); PRINTSTACK
#define NULLADDR   printf("NULLADDR  ptr=%X\n", ptr); PRINTSTACK

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

  n->size          = size;
  n->ptr           = ptr;
  n->state         = STATE_UNDEF;
  n->next          = NULL;
  n->prev          = NULL;
  n->freedTime     = 0;
  n->allocatedFrom = walkStack(storeAddr, TRUE);

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

  n->freedFrom = walkStack(storeAddr, TRUE);

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
    if(p->freedFrom)     freeStacktrace(p->freedFrom);
    if(p->allocatedFrom) freeStacktrace(p->allocatedFrom);
    realfree(p->ptr);
    realfree(p);
  }
}

static void
freeStacktrace(stacktrace *p)
{
  while(p) {
    stacktrace *n = p->next;
    if(p->fname) realfree(p->fname);
    if(p->sname) realfree(p->sname);
    realfree(p);
    p = n;
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

  switch(mallocState) {
  case UNINITIALIZED:
    LOCK;
    mallocState = INITIALIZING;
    if(loadLibC(LIBC)) {
      exit(-1);
    }
    mallocState = INITIALIZED;
    UNLOCK;

  case INITIALIZED:
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
    break;
    
  case INITIALIZING:
    if(size == 0) return NULL;
    return staticMalloc(size);
    break;
  }

  return NULL;
}

void
free(void *ptr)
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
  realfree(ptr);
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

static stacktrace *
walkStack(stacktrace *(*fn)(void *pc), int storeit)
{
#ifdef SOLARIS
  struct frame *sp = NULL;
  jmp_buf       env;
  int           i = 0;
  stacktrace   *st = NULL, *sto = NULL;
  
  FLUSHWIN();
  (void) setjmp(env);
  sp = (struct frame *) env[FRAME_PTR_INDEX];
  
  for(i = 0 ; i < SKIP_FRAMES && sp ; i++)
    sp = (struct frame *)sp->fr_savfp;
  
  while(sp && sp->fr_savpc) {
    stacktrace *s = (*fn)((void *)(sp->fr_savpc));
    if((storeit == TRUE) && s) {
      if(!sto) {
	sto = s;
	st  = s;
      } else {
	st->next = s;
	st       = s;
      }
    }
    sp = (struct frame *)sp->fr_savfp;
  }
  
  return sto;
#endif

  return NULL;
}

static stacktrace *
storeAddr(void *pc)
{
#ifdef SOLARIS
  Dl_info     info;
  int         l;
  stacktrace *s = (stacktrace *) realmalloc(sizeof(stacktrace));

  if(!s) {
    perror("storeAddr[chk.o]: malloc");
    return NULL;
  }

  s->pc = pc;

  if(dladdr(pc, & info) == 0) {
    return;
  }
  
  if (strstr(info.dli_fname, "ld.so.1"))
    return;

  s->saddr = info.dli_saddr;

  l = strlen(info.dli_fname);
  s->fname = realmalloc(l + 1); 
  (void) strncpy(s->fname, info.dli_fname, l);

  l = strlen(info.dli_sname);
  s->sname = realmalloc(l + 1); 
  (void) strncpy(s->sname, info.dli_sname, l);

  return s;
#else
  return NULL;
#endif
}

static void
printStacktrace(stacktrace *s) 
{
  if(!s) {
    (void) printf("\tstack trace not available.\n");
    return;
  }

  for( ; s ; s = s->next) {
    (void) printf("\t%s:%s+0x%x\n", 
		  s->fname ? s->fname : "unknown",
		  s->sname ? s->sname : "unknown",
		  (unsigned int)s->pc - (unsigned int)s->saddr);
  }
}

static stacktrace *
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
  return NULL;
}

static char    staticMem[STATICMEMSIZE];
static size_t  staticOffset = 0;

static void *
staticMalloc(size_t size)
{
  size_t  ep = size + ALIGNBOUND - (size % ALIGNBOUND) - 1;
  void   *p = NULL;

  LOCK;
  if(staticOffset + ep > STATICMEMSIZE) {
    errno = ENOMEM;
    UNLOCK;
    return NULL;
  }
  staticOffset += ep;
  p = (void *) (staticMem + staticOffset);
  UNLOCK;

  return p;
}

static void *
staticFree(void *ptr)
{
  if((ptr >= (void *)staticMem) && 
     (ptr <= (void *)staticMem + STATICMEMSIZE - 1)) {
    /* we don't really "free" this stuff: it's static */
  }
}
