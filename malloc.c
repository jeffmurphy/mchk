/* Copyright (c) 1998, 1999 Nickel City Software */

#define __MALLOC_C__
#include "malloc.h"

static char         staticMem[STATICMEMSIZE];

static void *
staticMalloc(size_t size)
{
  size_t  ep = size + ALIGNBOUND - (size % ALIGNBOUND) - 1;
  void   *p = NULL;

  LOCK;
  if(staticOffset + ep > STATICMEMSIZE) {
    errno = ENOMEM;
    UNLOCK;
    return NULL;
  }
  staticOffset += ep;
  p = (void *) (staticMem + staticOffset);
  UNLOCK;

  return p;
}


void *
malloc(size_t size)
{
  void *ptr = NULL;

  switch(mallocState) {
  case UNINITIALIZED:
    LOCK;
    mallocState = INITIALIZING;
    UNLOCK;
    if(loadLibC(LIBC)) {
      exit(-1);
    }
    LOCK;
    mallocState = INITIALIZED;
    UNLOCK;

  case INITIALIZED:
    if(size == 0) return (void *) NULL;
    size += REDZONESIZE * 2; 
    ptr = realmalloc(size);
    if(! ptr) return ptr;
    
    memset(ptr, 0, size);
    
    if(addAlloc(size, ptr) != 0) {
      realfree(ptr);
      return NULL;
    }
    
    return ptr + REDZONESIZE;
    break;
    
  case INITIALIZING:
    if(size == 0) return NULL;
    return staticMalloc(size);
    break;
  }

  return NULL;
}



