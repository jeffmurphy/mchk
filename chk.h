/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __chk_h__
# define __chk_h__

# include <stdio.h>
# include <stdlib.h>

# undef EXTERN
# ifdef __CHK_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

# include "stacktrace.h"
# include "list.h"
# include "lock.h"
# include "malloc.h"
# include "version.h"

EXTERN void  free(void *ptr);
EXTERN void  chkexit();
EXTERN void  chksetup();
EXTERN void  rchk(void *sp, void *ptr, size_t len);
EXTERN void  wchk(void *sp, void *ptr, size_t len);
EXTERN void  dumpAllocList();

# ifdef DEBUG
#  define D(X) printf X
# else
#  define D(X)
# endif


# ifndef FALSE
#  define FALSE 0
#  define TRUE !FALSE
# endif

# define BADFREE       printf("BADFREE    ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); PRINTSTACK
# define REFREE(X)     printf("REFREE     ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define READFREE(X)   printf("READFREE   ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define WRITEFREE(X)  printf("WRITEFREE  ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define BADADDR       printf("BADADDR    ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); PRINTSTACK
# define BADWRITE      printf("BADWRITE   ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); PRINTSTACK
# define BADREAD(X)    printf("BADREAD    ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define UNDERWRITE(X) printf("UNDERWRITE ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define OVERWRITE(X)  printf("OVERWRITE  ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define UNDERREAD(X)  printf("UNDERREAD  ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define OVERREAD(X)   printf("OVERREAD   ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); FULLTRACE(X)
# define NULLADDR      printf("NULLADDR   ptr=%X detected by %s\n", \
                              ptr, __FUNCTION__); PRINTSTACK

#endif /* __chk_h__ */
