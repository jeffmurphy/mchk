/* Copyright (c) 1998, 1999 Nickel City Software */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef DO_STACK_TRACE
# include <setjmp.h>
#endif

#ifdef SOLARIS
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
static pthread_t       heldBy = 0;
# define LDBG(X) 
# define LOCK                                              \
{                                                          \
  LDBG(("LOCK[%u]: pending\n", pthread_self())); \
  if(heldBy != pthread_self()) {                           \
     int __ptmlret = pthread_mutex_lock(&lock);            \
     LDBG(("LOCK[%u]: aquired\n", pthread_self())); \
     if(__ptmlret) {                                       \
        printf("chk.o: pthread_mutex_lock returned %d\n",  \
               __ptmlret);                                 \
        exit(-1);                                          \
     }                                                     \
     heldBy = pthread_self();                              \
   }                                                       \
}
# define UNLOCK                                             \
{                                                           \
  LDBG(("UNLOCK[%u]: release\n", pthread_self())); \
  if(heldBy == pthread_self()) {                            \
     int __ptmulret;                                        \
     heldBy = 0;                                            \
     __ptmulret = pthread_mutex_unlock(&lock);              \
     LDBG(("UNLOCK[%u]: released\n", pthread_self())); \
     if(__ptmulret) {                                       \
        printf("chk.o: pthread_mutex_unlock returned %d\n", \
               __ptmulret);                                 \
        exit(-1);                                           \
     }                                                      \
  } else {                                                  \
     printf("chk.o: mutex_unlock: thread %u doesn't hold the lock. (held by %u)\n", \
            pthread_self(), heldBy);                        \
     return;                                                \
  }                                                         \
}
#else
# define LOCK
# define UNLOCK
#endif

#ifndef FALSE
# define FALSE 0
# define TRUE !FALSE
#endif

#define PRINTSTACK                      \
{                                       \
  printf("\terror occurred at:\n");     \
  walkStack(printAddr, FALSE);          \
}
#define STORESTACK walkStack(storeAddr, TRUE)
#define FULLTRACE(X)                    \
{                                       \
  printf("\terror occurred at:\n");     \
  walkStack(printAddr, FALSE);          \
  printf("\tallocated from:\n");        \
  printStacktrace(X->allocatedFrom);    \
  if(X->freedFrom) {                    \
    printf("\tfreed from:\n");          \
    printStacktrace(X->freedFrom);      \
  }                                     \
}

#define BADFREE       printf("BADFREE   ptr=%X\n", ptr); PRINTSTACK
#define REFREE(X)     printf("REFREE    ptr=%X\n", ptr); FULLTRACE(X)
#define READFREE(X)   printf("READFREE  ptr=%X\n", ptr); FULLTRACE(X)
#define WRITEFREE(X)  printf("WRITEFREE ptr=%X\n", ptr); FULLTRACE(X)
#define BADADDR       printf("BADADDR   ptr=%X\n", ptr); PRINTSTACK
#define BADWRITE      printf("BADWRITE  ptr=%X\n", ptr); PRINTSTACK
#define BADREAD(X)    printf("BADREAD   ptr=%X\n", ptr); FULLTRACE(X)
#define UNDERWRITE(X) printf("UNDERWRITE ptr=%X\n", ptr); FULLTRACE(X)
#define OVERWRITE(X)  printf("OVERWRITE ptr=%X\n", ptr); FULLTRACE(X)
#define UNDERREAD(X)  printf("UNDERREAD ptr=%X\n", ptr); FULLTRACE(X)
#define OVERREAD(X)   printf("OVERREAD  ptr=%X\n", ptr); FULLTRACE(X)
#define NULLADDR      printf("NULLADDR  ptr=%X\n", ptr); PRINTSTACK

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
    return NULL;
  }

  done = 0;
  p    = list;

  /* we check to see if the ptr is WITHIN a known block of memory */

  while(!done) {
    if(!p) {
      done = 1;
    } else {
      if((p->ptr <= ptr) && (ptr <= (p->ptr + p->size))) {
	done = 1;
      }
    }
    if(!done) p = p->next;
  }

  if(!p) {  /* didnt find it */
    return NULL;
  }
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
    freeStacktraceNode(p);
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
    UNLOCK;
    if(loadLibC(LIBC)) {
      exit(-1);
    }
    LOCK;
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
      REFREE(p);
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


static void
trylookup(void *pc)
{
  Dl_info     info;

  if(dladdr(pc, & info) == 0) {
    (void) printf("\t{unknown}:{unknown} [0x%X]\n", (int)pc);
    return;
  }
  
  (void) printf("\t<%s>:<%s>+0x%x [0x%X]\n", 
		info.dli_fname,
		info.dli_sname,
		(unsigned int)pc - (unsigned int)info.dli_saddr,
		(unsigned int)pc);

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
#if defined(DO_STACK_TRACE)
# if defined(SOLARIS)
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
      D(("pc:%x fbase:%x saddr:%x\n",s->pc,s->fbase,s->saddr));
      if (s->next != 0) {
	D(("got a bad frame\n"));
	break;
      }
      D(("%s (%s)\n", s->sname, s->fname));
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
# elif defined(LINUX)
  jmp_buf       env;
  int           i = 0, done = FALSE;
  stacktrace   *st = NULL, *sto = NULL;
#define T unsigned int
  T         *next, *fp;

  setjmp(env);

  /* the linux jmpbuf.__jmpbuf looks like this (at least in kernel 2.0.x)
   * [0] : ptr to [2]
   * [1] : ptr to function
   * [2] : ptr to [4]
   * [3] : ptr to function
   * [4] : ...
   *
   * so we determine when we hit the end of the callback list by
   * checking if  "ptr to [x] < ptr to [2]".
   *
   * in addition, since the first thing on the stack is likely to 
   * be "rchk" or "wchk", we start at position 2 to get the routine
   * that called us.
   */

  next = (T *)&(env[1].__jmpbuf[0]);
  fp   = (T *)*(next + 1);
  while(!done) {
    stacktrace *s;
    next = (T *)*next;
    fp   = (T *)*(next + 1);
    if(*next) {
      s = (*fn)((void *)(fp));
      D((">>>> [%8.8x -> %8.8x] [%8.8x] ", next, *next, fp));
      if(storeit == TRUE) {
	if(s) {
	  if(!sto) {
	    sto = s;
	    st  = s;
	  } else {
	    st->next = s;
	    st       = s;
	  }
	} else {
	  printf("[chk.o::walkStack] error: storeit==true but s==null\n");
	}
      } else {
	if(s) freeStacktraceNode(s);
      }
      
    } else
      done = TRUE;
  }

  return sto;
# endif /* OS SPECIFIC TESTS */


#endif /* DO_STACK_TRACE */


  return NULL;
}

static stacktrace *
storeAddr(void *pc)
{
#if (defined(SOLARIS) || defined(LINUX)) && defined(DO_STACK_TRACE)
  Dl_info     info;
  int         l;
  stacktrace *s = (stacktrace *) realmalloc(sizeof(stacktrace));

  if(!s) {
    perror("storeAddr[chk.o]: malloc");
    return NULL;
  }

  s->pc = pc;

  if(dladdr(pc, & info) == 0) {
    s->fname = realmalloc(8);
    if(!s->fname) {
      realfree(s);
      return NULL;
    }
    s->sname = realmalloc(8);
    if(!s->sname) {
      realfree(s);
      return NULL;
    }
    strcpy(s->fname, "unknown");
    strcpy(s->sname, "unknown");
    return s;
  }
  
  if (strstr(info.dli_fname, "ld.so.1")) {
    realfree(s);
    return NULL;
  }

  s->saddr = info.dli_saddr;

  l = strlen(info.dli_fname);
  s->fname = realmalloc(l + 1); 
  if(!s->fname) {
    realfree(s);
    return NULL;
  }

  (void) strncpy(s->fname, info.dli_fname, l);

  l = strlen(info.dli_sname);
  s->sname = realmalloc(l + 1); 
  if(!s->sname) {
    realfree(s->fname);
    realfree(s);
    return NULL;
  }

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
    (void) printf("\t\tstack trace not available.\n");
    return;
  }
  
  for( ; s ; s = s->next) {
    (void) printf("\t\t<%s>:<%s> +0x%x\n", 
		  *(s->fname) ? s->fname : "unknown",
		  *(s->sname) ? s->sname : "unknown",
		  (unsigned int)s->pc - (unsigned int)s->saddr);
  }
}

static void
freeStacktraceNode(stacktrace *s) 
{
  if(s) {
    if(s->fname) realfree(s->fname);
    if(s->sname) realfree(s->sname);
    realfree(s);
  }
}

static stacktrace *
printAddr(void *pc)
{
#if defined(SOLARIS) || defined(LINUX)
  Dl_info info;
  
  if(dladdr(pc, & info) == 0) {
    (void) printf("\t\t<unknown>: 0x%x\n", (int)pc);
    return;
  }
  
  /*
   * filter out any mention of rtld.  We're hiding the cost
   * of the rtld_auditing here.
   */
  if (strstr(info.dli_fname, "ld.so.1"))
    return;
  
  (void) printf("\t\t%s:%s+0x%x\n", 
		info.dli_fname,
		info.dli_sname,
		(unsigned int)pc - (unsigned int)info.dli_saddr);
#endif
  return NULL;
}

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
