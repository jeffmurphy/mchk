#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>

#define MAXARGS 31

void
printthis(char *fmt, ...)
{
  va_list  arglist;
  va_start(arglist, MAXARGS);

  vprintf(fmt, arglist);
}

int
main()
{
  printf("varargs test\n");

  printthis("%d %d %d", 1, 2, 3);

  exit(0);
}
