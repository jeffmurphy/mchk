/* Copyright (c) 1998, 1999 Nickel City Software */

#define __LOCK_C__
#include "lock.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t       heldBy = 0;
unsigned int    sema   = 0;
