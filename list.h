/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __LIST_H__
# define __LIST_H__

# include <stdio.h>
# include <sys/types.h>
# include <sys/time.h>
# include <unistd.h>

# include "stacktrace.h"

# undef EXTERN
# ifndef __LIST_C__
#  define EXTERN extern
# else
#  define EXTERN
# endif

# ifndef FALSE
#  define FALSE 0
#  define TRUE !FALSE
# endif

typedef struct _memnode memnode;
struct _memnode {
  size_t   size;
  void    *ptr;
# define STATE_UNDEF -1
# define STATE_DEF    1
  char           state;
  long           freedTime;

  stacktrace    *allocatedFrom;
  stacktrace    *freedFrom;

  memnode       *prev;
  memnode       *next;
};



EXTERN void         closeHole(memnode *p);
EXTERN void         freeMemnode(memnode *p);
EXTERN int          addFree(memnode *n);
EXTERN int          addAlloc(size_t size, void *ptr);
EXTERN memnode     *findMemnode(memnode *list, void *ptr);

# ifdef __LIST_C__
memnode     *allocList = NULL;
memnode     *freeList  = NULL;
# else
extern memnode     *allocList;
extern memnode     *freeList;
# endif

#endif /* __LIST_H__ */
