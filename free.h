#ifndef __FREE_H__
# define __FREE_H__

# include "list.h"
# include "malloc.h"
# include "chk.h"


# undef EXTERN
# ifdef __FREE_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

EXTERN void staticFree(void *ptr);
EXTERN void free(void *ptr);

#endif /* __FREE_H__ */
