#include <stdio.h>
#include <chk.h>

#if 0
int
foo(void *p, unsigned int len, unsigned char t, int z, int y, int x, int w,
    int m, int n)
{
  printf("%x %d %d %d %d %d %d %d %d\n",
	 p, len, t, z, y, x, w, m, n);
  return 1;
}
#endif

int
main()
{
  char a;
  int  b, b2;
  short c;
  char *m;

  printf ("\ta (%x) = 123\n", &a);
  a = 123;

  printf ("\tb (%x) = 0x12345678\n", &b);
  b = 0x12345678;
  printf ("\tc (%x) = 0x4321\n", &c);
  c = 0x4321;
  printf ("\tb2 (%x) = b (%x)\n", &b2, &b);
  b2= b;
  printf ("\tm (%x) = malloc(10)\n", &m);

  m = (char *) malloc(10);

  printf ("\tm (%x) = %X\n", &m, m);
  printf ("\tm[10] = 1\n");
  m[10] = 1; /* overwrite */

  a = m[10]; /* overread */
  printf("\tm[10] = %d \n", m[10]);

  /* should complain on exit of a memory leak of 10 bytes */
  exit(0);
}


