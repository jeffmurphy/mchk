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
# define REDZONESIZE 8 /* # of bytes - try to preserve alignment */
# define LIBC "/lib/libc.so"

typedef struct _memnode memnode;
struct _memnode {
  size_t   size;
  void    *ptr;
# define STATE_UNDEF 0
# define STATE_DEF   1
  int            state;
  long           freedTime;

  memnode       *prev;
  memnode       *next;
};

static void *(*realmalloc)(size_t) = 0;
static void  (*realfree)(void *)   = 0;
static char malloc_init            = 0;
static int  semaphore              = 0;

static memnode *allocList;
static memnode *freeList;
static long     freeAge = 60;

static void     closeHole(memnode *p);
static void     freeMemnode(memnode *p);
static int      addFree(memnode *n);
static int      addAlloc(size_t size, void *ptr);
static memnode *findMemnode(memnode *list, void *ptr);

static int      loadLibC(char *libc);

static int      walkStack(void);
static void     printAddr(void *);

#endif

#ifdef  __cplusplus
}
#endif

#endif /* __chk_h__ */
