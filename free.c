/* Copyright (c) 1998, 1999 Nickel City Software */

#define __FREE_C__
#include "free.h"
#include "list.h"
#include "malloc.h"
#include "chk.h"

extern char         staticMem[STATICMEMSIZE];

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

  /* we don't free memory assigned from our static pool */
#define T (unsigned int)
  if((T ptr >= T staticMem) && (T ptr <= (T staticMem + STATICMEMSIZE)))
    return;

  ptr -= REDZONESIZE; /* move to beginning of redzone */

  LOCK;

  /* search on the list of known allocated addresses for
   * this address 
   */
  for(p = allocList ; p && (p->ptr != ptr) ; p = p->next);
  if(p) {
    /* if we found a known (previously allocated) address,
     * then remove it from the allocList and add it to the
     * freeList. 
     */
    if(p == allocList) allocList = p->next;
    closeHole(p);
    addFree(p);
  } else {
    /* otherwise, search the list of known free addresses for
     * this address to check for double frees.
     */
    for(p = freeList ; p && (p->ptr != ptr) ; p = p->next);
    if(p) {
      REFREE(p);
    } else {
      BADFREE;
    }
  }
  UNLOCK;

  /* something to note: we didn't really free anything. this is
   * done so that we can check accesses to previously free'd memory.
   * we do this by not immediately freeing memory (because then malloc
   * might hand it back out again) and instead we let it "age". it will
   * eventually be freed in list.c freeMemnode()
   */
}

