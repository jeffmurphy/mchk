/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __MALLOC_H__
# define __MALLOC_H__

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "lock.h"

# undef EXTERN
# ifdef __MALLOC_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif


# ifndef RTLD_NOW
#  define RTLD_NOW 1
# endif

# define MALLOC_SYM "malloc"
# define FREE_SYM   "free"

  /* on linux, you might need to symlink this */

# ifdef SOLARIS
#  define LIBC          "/lib/libc.so"
# else
#  define LIBC          "/lib/libc.so.6"
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

EXTERN void       *malloc(size_t size);
EXTERN void       *(*realmalloc)(size_t);
EXTERN void        (*realfree)(void *);
EXTERN size_t      staticOffset;

#endif /* __MALLOC_H__ */

