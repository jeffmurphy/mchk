/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __MALLOC_H__
# define __MALLOC_H__

# include <stdio.h>
# include <errno.h>
# include <sys/types.h>
# include <dlfcn.h>

# include "loadlibc.h"
# include "lock.h"

# undef EXTERN
# ifdef __MALLOC_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

# define REDZONESIZE   8 /* # of bytes - try to preserve alignment */
# define STATICMEMSIZE 10000
# define ALIGNBOUND    8

typedef enum { UNINITIALIZED, INITIALIZING, INITIALIZED } mchkState_t;

# ifdef __MALLOC_C__
mchkState_t mallocState = UNINITIALIZED;
# else
EXTERN mchkState_t mallocState;
# endif

  /* on linux, you might need to symlink this */

# ifdef SOLARIS
#  define LIBC          "/lib/libc.so"
# else
#  if 1
#   define LIBC          "/lib/libc.so.6"
#  else
#   define LIBC          "/usr/src/redhat/SOURCES/glibc/glibc-2.0.7/libc.so"
#  endif
# endif

EXTERN void       *malloc(size_t size);
EXTERN size_t      staticOffset;

# ifdef __MALLOC_C__
void              *(*realmalloc)(size_t) = 0;
# else
EXTERN void       *(*realmalloc)(size_t);
# endif

#endif /* __MALLOC_H__ */

