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
EXTERN pthread_mutex_t lock;
EXTERN pthread_t       heldBy;
#  define LDBG(X) /* printf */
#  define LOCK                                              \
{                                                          \
  LDBG(("LOCK[%u]: pending\n", pthread_self())); \
  if(heldBy != pthread_self()) {                           \
     int __ptmlret = pthread_mutex_lock(&lock);            \
     LDBG(("LOCK[%u]: aquired\n", pthread_self())); \
     if(__ptmlret) {                                       \
        printf("chk.o: pthread_mutex_lock returned %d\n",  \
               __ptmlret);                                 \
        exit(-1);                                          \
     }                                                     \
     heldBy = pthread_self();                              \
   }                                                       \
}
#  define UNLOCK                                             \
{                                                           \
  LDBG(("UNLOCK[%u]: release\n", pthread_self())); \
  if(heldBy == pthread_self()) {                            \
     int __ptmulret;                                        \
     heldBy = 0;                                            \
     __ptmulret = pthread_mutex_unlock(&lock);              \
     LDBG(("UNLOCK[%u]: released\n", pthread_self())); \
     if(__ptmulret) {                                       \
        printf("chk.o: pthread_mutex_unlock returned %d\n", \
               __ptmulret);                                 \
        exit(-1);                                           \
     }                                                      \
  } else {                                                  \
     printf("chk.o: mutex_unlock: thread %u doesn't hold the lock. (held by %u)\n", \
            pthread_self(), heldBy);                        \
     return;                                                \
  }                                                         \
}
# else /* ! THREAD_SAFE */
#  define LOCK
#  define UNLOCK
# endif

#endif /* __LOCK_H__ */
