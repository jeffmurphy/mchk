/* Copyright (c) 1998, 1999 Nickel City Software */

#define __STACKTRACE_C__
#include "stacktrace.h"

/* the following code was partially adapted from a SUN example source file
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 *
 * #pragma ident   "@(#)print_my_stack.c   1.3     97/04/28 SMI"
 */


stacktrace *
walkStack(stacktrace *(*fn)(void *pc), int storeit)
{
#if defined(DO_STACK_TRACE)
# if defined(SOLARIS)
  struct frame *sp = NULL;
  jmp_buf       env;
  int           i = 0;
  stacktrace   *st = NULL, *sto = NULL;

  FLUSHWIN();
  (void) setjmp(env);
  sp = (struct frame *) env[FRAME_PTR_INDEX];
  
  for(i = 0 ; i < SKIP_FRAMES && sp ; i++)
    sp = (struct frame *)sp->fr_savfp;
  
  while(sp && sp->fr_savpc) {
    stacktrace *s = (*fn)((void *)(sp->fr_savpc));
    if((storeit == TRUE) && s) {
      D(("pc:%x fbase:%x saddr:%x\n",s->pc,s->fbase,s->saddr));
      if (s->next != 0) {
	D(("got a bad frame\n"));
	break;
      }
      D(("%s (%s)\n", s->sname, s->fname));
      if(!sto) {
	sto = s;
	st  = s;
      } else {
	st->next = s;
	st       = s;
      }
    }
    sp = (struct frame *)sp->fr_savfp;
  }

  return sto;
# elif defined(LINUX)
  jmp_buf       env;
  int           i = 0, done = FALSE;
  stacktrace   *st = NULL, *sto = NULL;
#define T unsigned int
  T         *next, *fp;

  setjmp(env);

  /* the linux jmpbuf.__jmpbuf looks like this (at least in kernel 2.0.x)
   * [0] : ptr to [2]
   * [1] : ptr to function
   * [2] : ptr to [4]
   * [3] : ptr to function
   * [4] : ...
   *
   * so we determine when we hit the end of the callback list by
   * checking if  "ptr to [x] < ptr to [2]".
   *
   * in addition, since the first thing on the stack is likely to 
   * be "rchk" or "wchk", we start at position 2 to get the routine
   * that called us.
   */

  next = (T *)&(env[1].__jmpbuf[0]);
  fp   = (T *)*(next + 1);
  while(!done) {
    stacktrace *s;
    next = (T *)*next;
    fp   = (T *)*(next + 1);
    if(*next) {
      s = (*fn)((void *)(fp));
      if(storeit == TRUE) {
	if(s) {
	  if(!sto) {
	    sto = s;
	    st  = s;
	  } else {
	    st->next = s;
	    st       = s;
	  }
	} else {
	  printf("[chk.o::walkStack] error: storeit==true but s==null\n");
	}
      } else {
	if(s) freeStacktraceNode(s);
      }
      
    } else
      done = TRUE;
  }

  return sto;
# endif /* OS SPECIFIC TESTS */


#endif /* DO_STACK_TRACE */


  return NULL;
}

stacktrace *
storeAddr(void *pc)
{
#if (defined(SOLARIS) || defined(LINUX)) && defined(DO_STACK_TRACE)
  Dl_info     info;
  int         l;
  stacktrace *s = (stacktrace *) realmalloc(sizeof(stacktrace));

  if(!s) {
    perror("storeAddr[chk.o]: malloc");
    return NULL;
  }

  s->pc = pc;

  if(dladdr(pc, & info) == 0) {
    s->fname = realmalloc(8);
    if(!s->fname) {
      realfree(s);
      return NULL;
    }
    s->sname = realmalloc(8);
    if(!s->sname) {
      realfree(s);
      return NULL;
    }
    strcpy(s->fname, "unknown");
    strcpy(s->sname, "unknown");
    return s;
  }
  
  if (strstr(info.dli_fname, "ld.so.1")) {
    realfree(s);
    return NULL;
  }

  s->saddr = info.dli_saddr;

  l = strlen(info.dli_fname);
  s->fname = realmalloc(l + 1); 
  if(!s->fname) {
    realfree(s);
    return NULL;
  }

  (void) strncpy(s->fname, info.dli_fname, l);

  l = strlen(info.dli_sname);
  s->sname = realmalloc(l + 1); 
  if(!s->sname) {
    realfree(s->fname);
    realfree(s);
    return NULL;
  }

  (void) strncpy(s->sname, info.dli_sname, l);

  return s;
#else
  return NULL;
#endif
}

void
printStacktrace(stacktrace *s) 
{
  if(!s) {
    (void) printf("\t\tstack trace not available.\n");
    return;
  }
  
  for( ; s ; s = s->next) {
# if defined(DEMANGLE_GNU_CXX)
    char   *dem = NULL; /* GNU CXX */
    dem = cplus_demangle(s->sname, DMGL_DEFAULT);
# else
#   define DMGL_BUFFER_SIZE 1024
    char    dem[DMGL_BUFFER_SIZE];  /* SUN CXX */
    if(cplus_demangle(s->sname, dem, DMGL_BUFFER_SIZE) != 0)
      strcpy(dem, s->sname);
# endif

    (void) printf("\t\t<%s>:", *(s->fname) ? s->fname : "unknown");
    if(dem) {
      (void) printf("<%s>", dem);
# if defined(DEMANGLE_GNU_CXX)
      free(dem);
# endif
    } else {
      (void) printf("<%s>", *(s->sname) ? s->sname : "unknown");
    }
    (void) printf(" +0x%x\n", 
		  (unsigned int)s->pc - (unsigned int)s->saddr);
  }
}


stacktrace *
printAddr(void *pc)
{
#if defined(SOLARIS) || defined(LINUX)
# ifdef DEMANGLE_GNU_CXX
  char   *dem = NULL; /* GNU CXX */
# else
#  define DMGL_BUFFER_SIZE 1024
  char    dem[DMGL_BUFFER_SIZE];  /* SUN CXX */
# endif

  Dl_info info;
  
  if(dladdr(pc, & info) == 0) {
    (void) printf("\t\t<unknown>:<unknown> 0x%x\n", (int)pc);
    return;
  }
  
  /*
   * filter out any mention of rtld.  We're hiding the cost
   * of the rtld_auditing here.
   */
  if (strstr(info.dli_fname, "ld.so.1")) return;

  /*dem = demangle_it(info.dli_sname);*/
#if   defined(DEMANGLE_GNU_CXX)
  dem = cplus_demangle(info.dli_sname, DMGL_DEFAULT);
#elif defined(DEMANGLE_SUN_CXX)
  if(cplus_demangle(info.dli_sname, dem, DMGL_BUFFER_SIZE) != 0)
    strcpy(dem, info.dli_sname);
#endif

  (void) printf("\t\t<%s>:", 
		*(info.dli_fname) ? info.dli_fname : "unknown");
  if(dem) {
    (void) printf("<%s>", dem);
#if defined(DEMANGLE_GNU_CXX)
    free(dem);
#endif
  } else {
    (void) printf("<%s>", info.dli_sname);
  }

  (void) printf("+0x%x\n", 
		(unsigned int)pc - (unsigned int)info.dli_saddr);
#endif
  return NULL;
}

