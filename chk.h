/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __chk_h__
#define __chk_h__

#include <stdio.h>
#include <stdlib.h>

#undef EXTERN
#ifdef __MCHK_CORE___
# define EXTERN
#else
# define EXTERN extern
#endif

#ifdef  __cplusplus
extern "C" {
#endif

EXTERN void *malloc(size_t size);
EXTERN void  free(void *ptr);
EXTERN void  chkexit();

#ifdef __MCHK_CORE__
# define REDZONESIZE   8 /* # of bytes - try to preserve alignment */
# define STATICMEMSIZE 10000
# define ALIGNBOUND    8

  /* on linux, you might need to symlink this */

# ifdef SOLARIS
#  define LIBC          "/lib/libc.so"
# else
#  define LIBC          "/lib/libc.so.6"
# endif

typedef struct _stacktrace stacktrace;
struct _stacktrace {
  void       *pc;
  char       *fname; /* filename */
  void       *fbase; /* base address of object */
  char       *sname; /* symbol name nearest to address */
  void       *saddr; /* actual address of symbol */
  stacktrace *next;
};

typedef struct _memnode memnode;
struct _memnode {
  size_t   size;
  void    *ptr;
# define STATE_UNDEF 0
# define STATE_DEF   1
  int            state;
  long           freedTime;

  stacktrace    *allocatedFrom;
  stacktrace    *freedFrom;

  memnode       *prev;
  memnode       *next;
};

typedef enum { UNINITIALIZED, INITIALIZING, INITIALIZED } mchkState_t;

static void       *(*realmalloc)(size_t) = 0;
static void        (*realfree)(void *)   = 0;
static mchkState_t  mallocState          = UNINITIALIZED;

static memnode *allocList;
static memnode *freeList;
static long     freeAge = 60;

static void     closeHole(memnode *p);
static void     freeMemnode(memnode *p);
static int      addFree(memnode *n);
static int      addAlloc(size_t size, void *ptr);
static memnode *findMemnode(memnode *list, void *ptr);

static int      loadLibC(char *libc);

static stacktrace *walkStack(stacktrace *(*fn)(void *pc), int storeit);
static stacktrace *storeAddr(void *pc);
static stacktrace *printAddr(void *pc);
static void        freeStacktrace(stacktrace *p);
static void        printStacktrace(stacktrace *s);

static void    *staticMalloc(size_t size);

static char    staticMem[STATICMEMSIZE];
static size_t  staticOffset = 0;


#endif

#ifdef  __cplusplus
}
#endif

#endif /* __chk_h__ */
