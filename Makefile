CC=gcc
AS=as
#AS=/usr/bin/as
CFLAGS= -g -I. -DSOLARIS -DTHREAD_SAFE -D_REENTRANT # -DDEBUG
LIBS  = -L. -lmchk -ldl

MCHKSRC = chk.c
MCHKOBJ = $(MCHKSRC:%.c=%.o)
MCHKASM = $(MCHKSRC:%.c=%.s)

all:	libmchk.a t1 t2

t1:	t1.o chk.o
	$(CC) -o t1 t1.o $(LIBS)

t2:	t2.o chk.o
	$(CC) -o t2 t2.o $(LIBS)

libmchk.a:	$(MCHKOBJ)
	@echo "Building libmchk.a .."
	ar rvf libmchk.a $(MCHKOBJ)

malloc.o:	malloc.c
	$(CC) $(CFLAGS) -S malloc.c -o malloc.s
	@$(AS) -Qy -o malloc.o malloc.s
	@rm -f malloc.s

chk.o:	chk.c
	$(CC) $(CFLAGS) -S chk.c -o chk.s
	@$(AS) -Qy -o chk.o chk.s
	@rm -f chk.s

clean:
	rm -f *.o *.s *.S_orig *~ *.a t1 t2



