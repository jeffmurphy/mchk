/* Copyright (c) 1998, 1999 Nickel City Software */

#define __LIST_C__
#include "list.h"

static long         freeAge = 60;


int
addFree(memnode *n)
{
  memnode        *p;
  struct timeval  tv;

  if(gettimeofday(&tv, NULL)) {
    perror("chk.o: gettimeofday");
  } else {
    n->freedTime = tv.tv_sec;
  }

  /* add the new node to the freeList */

  if(!freeList) {
    freeList = n;
  } else {
    for(p = freeList ; p->next ; p = p->next);
    p->next = n;
    n->prev = p;
  }

  n->freedFrom = walkStack(storeAddr, TRUE);

  /* free any nodes that are too old */

  for(p = freeList ; p ; p = p->next) {
    if(p->freedTime < (tv.tv_sec - freeAge)) {
      if(p = freeList) freeList = p->next;
      closeHole(p);
      freeMemnode(p);
    }
  }

  return 0;
}


void
closeHole(memnode *p) 
{
  if(!p) return;

  if(p->prev) {
    if(p->next) {
      p->prev->next = p->next;
      p->next->prev = p->prev;
    } else {
      p->prev->next = NULL;
    }
  } else {
    if(p->next) {
      p->next->prev = NULL;
    }
  }

  p->next = NULL;
  p->prev = NULL;
}


void
freeMemnode(memnode *p)
{
  if(p && p->ptr) {
    if(p->freedFrom)     freeStacktrace(p->freedFrom);
    if(p->allocatedFrom) freeStacktrace(p->allocatedFrom);
    realfree(p->ptr);
    realfree(p);
  }
}

memnode *
findMemnode(memnode *list, void *ptr)
{
  memnode *p;
  int      done;

  if(!ptr) {
    return NULL;
  }

  done = 0;
  p    = list;

  /* we check to see if the ptr is WITHIN a known block of memory */

  while(!done) {
    if(!p) {
      done = 1;
    } else {
      if((p->ptr <= ptr) && (ptr <= (p->ptr + p->size))) {
	done = 1;
      }
    }
    if(!done) p = p->next;
  }

  if(!p) {  /* didnt find it */
    return NULL;
  }
  return p;
}

int
addAlloc(size_t size, void *ptr)
{
  memnode *n = realmalloc(sizeof(memnode));
  memnode *p;

  if(!n) return -1;

  n->size          = size;
  n->ptr           = ptr;
  n->state         = STATE_UNDEF;
  n->next          = NULL;
  n->prev          = NULL;
  n->freedTime     = 0;
  n->allocatedFrom = walkStack(storeAddr, TRUE);

  LOCK;
  if(! allocList) {
    allocList = n;
  } else {
    for(p = allocList ; p->next ; p = p->next);
    n->prev = p;
    p->next = n;
  }
  UNLOCK;

  return 0;
}

void
freeStacktraceNode(stacktrace *s) 
{
  if(s) {
    if(s->fname) realfree(s->fname);
    if(s->sname) realfree(s->sname);
    realfree(s);
  }
}

void
freeStacktrace(stacktrace *p)
{
  while(p) {
    stacktrace *n = p->next;
    freeStacktraceNode(p);
    p = n;
  }
}
