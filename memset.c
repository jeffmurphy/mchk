/* Copyright (c) 1998, 1999 Nickel City Software */

#define __MEMSET_C__

#include "memset.h"
#include "list.h"
#include "lock.h"
#include "chk.h"

void *
_memset(void *ptr, int c, size_t len)
{
  memnode *n;

  if(mallocState != INITIALIZED) {
    if(!ptr) {
      printf("ERROR: memset ptr == NULL [mchk not yet initialized]\n");
      return;
    }
    return realmemset(ptr, c, len);
  }

  if(!ptr) { 
    NULLADDR;
    return NULL;
  }

  LOCK;
  n = findMemnode(allocList, ptr);
  /* if unknown addr: BADADDR or WRITEFREE */
  if(!n) {
    n = findMemnode(freeList, ptr);
    if(n) {
      WRITEFREE(n);
    } else {
      BADADDR;
    }
    UNLOCK;
    return;
  }

  n->state = STATE_DEF;

  /* if ptr or ptr+n is in a redzone: OVER/UNDER WRITE */

  if(ptr < (n->ptr + REDZONESIZE)) {
    UNDERWRITE(n);
  }
  else if(ptr >= (n->ptr + n->size - REDZONESIZE)) {
    OVERWRITE(n);
  }

  if((ptr+len) < (n->ptr + REDZONESIZE)) {
    UNDERWRITE(n);
  } 

  /* why ">" and not ">="? because we are really checking
   * (ptr+len-1). ptr = 10, len = 9. out value goes in 10-17. 
   * ptr+len = 18, so we subtract 1 off of it, or just use ">"
   */

  else if((ptr+len) > (n->ptr + n->size - REDZONESIZE)) {
    OVERWRITE(n);
  }

  UNLOCK;

  return realmemset(ptr, c, len);
}

void *
realmemset(void *ptr, int c, size_t n)
{
  if(ptr) {
    char *eptr = ptr + n;
    char *b    = ptr;
    while(b < eptr) {
      *(b++) = c;
    }
  }
}
