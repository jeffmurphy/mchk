/* Copyright (c) 1998, 1999 Nickel City Software */

#ifndef __LOCK_H__
# define __LOCK_H__

# undef EXTERN
# ifdef __LOCK_C__
#  define EXTERN
# else
#  define EXTERN extern
# endif

# ifdef THREAD_SAFE
#  include <pthread.h>

#  ifndef __LOCK_C__
EXTERN pthread_mutex_t lock;
EXTERN pthread_t       heldBy;
EXTERN unsigned int    sema;
#  endif

#  define LDBG(X)  /* printf ("%s %s %d ", __FILE__, __FUNCTION__, __LINE__); printf X /**/

#  define LOCK                                             \
{                                                          \
  LDBG(("LOCK[%u]: pending\n", pthread_self()));           \
  if(heldBy != pthread_self()) {                           \
     int __ptmlret = pthread_mutex_lock(&lock);            \
     LDBG(("LOCK[%u]: aquired\n", pthread_self()));        \
     if(__ptmlret) {                                       \
        printf("libmchk: pthread_mutex_lock returned %d\n",  \
               __ptmlret);                                 \
        exit(-1);                                          \
     }                                                     \
     heldBy = pthread_self();                              \
     sema = 1;                                             \
   } else {                                                \
        sema++;                                            \
        LDBG(("LOCK[%d]: already held. sema=%u\n",         \
              pthread_self(), sema));                      \
   }                                                       \
}

#  define UNLOCK                                            \
{                                                           \
  LDBG(("UNLOCK[%u]: release\n", pthread_self()));          \
  if(heldBy == pthread_self()) {                            \
     int __ptmulret;                                        \
     if(sema > 0) sema--;                                   \
     if(sema == 0) {                                        \
        heldBy = 0;                                         \
        __ptmulret = pthread_mutex_unlock(&lock);           \
       LDBG(("UNLOCK[%u]: released\n", pthread_self()));    \
       if(__ptmulret) {                                     \
          printf("libmchk: pthread_mutex_unlock returned %d\n", \
                 __ptmulret);                               \
          exit(-1);                                         \
       }                                                    \
     } else {                                               \
       LDBG(("UNLOCK[%u]: sema=%u\n", pthread_self(), sema)); \
     }                                                      \
  } else {                                                  \
     printf("libmchk: mutex_unlock: thread %u doesn't hold the lock. (held by %u)\n", \
            pthread_self(), heldBy);                        \
     return;                                                \
  }                                                         \
}
# else /* ! THREAD_SAFE */
#  define LOCK
#  define UNLOCK
# endif

#endif /* __LOCK_H__ */
