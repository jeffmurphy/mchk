/* Copyright (c) 1998, 1999 Nickel City Software */

#define __LOADLIBC_C__
#include "loadlibc.h"

/* the following concept (dyn loading libc instead of re-writting
 * malloc) was borrowed from ccmalloc.
 */

int
loadLibC(char *libc)
{
  void *handle;

  handle = dlopen(libc, RTLD_NOW);
  if(!handle) {
    perror("dlopen");
    return -1;
  }

  realmalloc = (void*(*)(size_t)) dlsym(handle, MALLOC_SYM);
  if(!realmalloc) {
    perror("dlsym[malloc]");
    dlclose(handle);
    return -1;
  }

  realfree = (void(*)(void*)) dlsym(handle, FREE_SYM);
  if(!realfree) {
    perror("dlsym[free]");
    dlclose(handle);
    return -1;
  }

  return 0;
}

