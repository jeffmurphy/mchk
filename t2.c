#include <stdio.h>
#include <stdlib.h>

int
main ()
{
  char *zone = (char *) malloc (20);
  char *ptr = NULL;
  int i;
  char c;
  
  c = zone[1];     /* error: read an uninitialized char */
  c = zone[-2];    /* error: read before the zone */
  zone[24] = ' ';  /* error: write after the zone */

  free(zone);
  zone[10] = 1;    /* error: writing to free'd memory */

  *ptr = 2;        /* error: use a NULL pointer,
		      must produce a core */
}
