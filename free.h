/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __FREE_H__
# define __FREE_H__

# undef EXTERN
# ifdef __FREE_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

EXTERN void  staticFree(void *ptr);
EXTERN void  free(void *ptr);

# ifdef __FREE_C__
void        (*realfree)(void *) = 0;
# else
EXTERN void (*realfree)(void *);
# endif

#endif /* __FREE_H__ */
