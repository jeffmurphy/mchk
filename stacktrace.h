/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __STACKTRACE_H__
# define __STACKTRACE_H__

# include <stdio.h>
# include <sys/types.h>
# include <dlfcn.h>
# include "malloc.h"
# include "demangle.h"

# undef EXTERN
# ifdef __STACKTRACE_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

#ifdef DO_STACK_TRACE
# include <setjmp.h>
#endif

# ifndef FALSE
#  define FALSE 0
#  define TRUE !FALSE
# endif

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

typedef struct _stacktrace stacktrace;
struct _stacktrace {
  void       *pc;
  char       *fname; /* filename */
  void       *fbase; /* base address of object */
  char       *sname; /* symbol name nearest to address */
  void       *saddr; /* actual address of symbol */
  stacktrace *next;
};

EXTERN stacktrace  *walkStack(stacktrace *(*fn)(void *pc), int storeit);
EXTERN stacktrace  *storeAddr(void *pc);
EXTERN stacktrace  *printAddr(void *pc);
EXTERN void         freeStacktrace(stacktrace *p);
EXTERN void         freeStacktraceNode(stacktrace *p);
EXTERN void         printStacktrace(stacktrace *s);

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


#endif /* __STACKTRACE_H__ */

