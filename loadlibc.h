/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __LOADLIBC_H__
# define __LOADLIBC_H__

# include <stdio.h>
# include <errno.h>
# include <sys/types.h>
# include <dlfcn.h>

# include "lock.h"
# include "malloc.h"
# include "free.h"

# undef EXTERN
# ifdef __LOADLIBC_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif


# ifndef RTLD_NOW
#  define RTLD_NOW 1
# endif

# define MALLOC_SYM "malloc"
# define FREE_SYM   "free"

EXTERN int loadLibC(char *libc);

#endif /* __LOADLIBC_H__ */
