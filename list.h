#ifndef __LIST_H__
# define __LIST_H__

# include <stdio.h>
# include <sys/types.h>
# include <sys/time.h>
# include <unistd.h>
# include "stacktrace.h"
# include "lock.h"

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
# define STATE_UNDEF 0
# define STATE_DEF   1
  int            state;
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

EXTERN memnode     *allocList;
EXTERN memnode     *freeList;

#endif /* __LIST_H__ */
