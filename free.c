/* Copyright (c) 1998, 1999 Nickel City Software */

#define __FREE_C__
#include "free.h"

void 
staticFree(void *ptr)
{
  /* we don't really "free" this stuff: it's static */
  return;
}

void
free(void *ptr)
{
  memnode *p;

  if(mallocState != INITIALIZED) return;

  ptr -= REDZONESIZE; /* move to beginning of redzone */

  LOCK;
  for(p = allocList ; p && (p->ptr != ptr) ; p = p->next);
  if(p) {
    if(p == allocList) allocList = p->next;
    closeHole(p);
    addFree(p);
  } else {
    for(p = freeList ; p && (p->ptr != ptr) ; p = p->next);
    if(p) {
      REFREE(p);
    } else {
      BADFREE;
    }
  }
  UNLOCK;
  realfree(ptr);
}

