/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __MEMSET_H__
# define __MEMSET_H__

# undef EXTERN
# ifdef __MEMSET_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

# include <sys/types.h>

EXTERN void *memset(void *ptr, int c, size_t n);
EXTERN void *realmemset(void *ptr, int c, size_t n);

#endif /* __MEMSET_H__ */
