#include <stdio.h>
#include <sys/types.h>
#ifndef __GNUC__
# include <sys/varargs.h>
#endif

void
printthis(char *fmt, ...)
{
  va_list  arglist;
  va_start(arglist, fmt);
  vfprintf(stdout, fmt, arglist);
}

int
main()
{
  printf("varargs test\n");

  printthis("%d %d %d", 1, 2, 3);

  exit(0);
}
