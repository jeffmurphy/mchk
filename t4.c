#include <stdio.h>

int
main(int ac, char **av)
{
  FILE *fp = NULL;
  char  b[1024];
  int   c = 0;

  if(ac != 2) {
    printf("usage: %s [filename]\n", av[0]);
    exit(-1);
  }

  fp = fopen(av[1], "r");
  if(!fp) {
    perror("fopen");
    exit(-1);
  }

  while((fscanf(fp, "%s", b) != EOF)) {
    c++;
  }
  fclose(fp);

  printf("scanned %d words from file %s.\n", 
	 c, av[0]);
  exit(0);
}
