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
static void        freeStacktraceNode(stacktrace *p);
static void        printStacktrace(stacktrace *s);

static void       *staticMalloc(size_t size);

static char    staticMem[STATICMEMSIZE];
static size_t  staticOffset = 0;

static char       *demangle_it (const char *mangled_name);

# ifdef DEMANGLE_GNU_CXX
  /* this stuff comes from GNU's demangle.h which doesnt appear to be
   * available by default on a system.
   */

  /* Options passed to cplus_demangle (in 2nd parameter). */

#  define DMGL_NO_OPTS    0               /* For readability... */
#  define DMGL_PARAMS     (1 << 0)        /* Include function args */
#  define DMGL_ANSI       (1 << 1)        /* Include const, volatile, etc */
#  define DMGL_JAVA       (1 << 2)        /* Demangle as Java */

#  define DMGL_AUTO       (1 << 8)
#  define DMGL_GNU        (1 << 9)
#  define DMGL_LUCID      (1 << 10)
#  define DMGL_ARM        (1 << 11)

#  define DMGL_DEFAULT (DMGL_PARAMS | DMGL_ANSI)

extern char *cplus_demangle(const char *sym, int flags);
# elif defined(DEMANGLE_SUN_CC)
#  include <demangle.h>
# endif

#endif

#ifdef  __cplusplus
}
#endif

#endif /* __chk_h__ */
