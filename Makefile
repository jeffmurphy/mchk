CXX=g++
CCC=$(CXX)
CC=gcc
#AS=as
AS=/usr/bin/as
SOLARIS = #-DSOLARIS
CFLAGS= -g -I. -DDO_STACK_TRACE $(SOLARIS) -DTHREAD_SAFE -D_REENTRANT # -DDEBUG
LIBS  = -L. -lmchk -lpthread -ldl

MCHKSRC = chk.c
MCHKOBJ = $(MCHKSRC:%.c=%.o)
MCHKASM = $(MCHKSRC:%.c=%.s)

all:	libmchk.a t1 t2 t3 t4 t4 t5

t1:	t1.o libmchk.a
	$(CC) $(CFLAGS) -o t1 t1.o $(LIBS)

t2:	t2.o libmchk.a
	$(CC) $(CFLAGS) -o t2 t2.o $(LIBS)

t3:	t3.o libmchk.a
	$(CXX) $(CFLAGS) -o t3 t3.o $(LIBS)

t4:	t4.o libmchk.a
	$(CC) $(CFLAGS) -o t4 t4.o $(LIBS)

t5:	t5.o libmchk.a
	$(CC) $(CFLAGS) -o t5 t5.o $(LIBS)

libmchk.a:	$(MCHKOBJ)
	@echo "Building libmchk.a .."
	ar rv libmchk.a $(MCHKOBJ)

malloc.o:	malloc.c
	$(CC) $(CFLAGS) -S malloc.c -o malloc.s
	@$(AS) -Qy -o malloc.o malloc.s
	@rm -f malloc.s

chk.o:	chk.c chk.h
	$(CC) $(CFLAGS) -S chk.c -o chk.s
	@$(AS) -Qy -o chk.o chk.s
	@rm -f chk.s

clean:
	rm -f *.o *.s *.S_orig *~ *.a t? core



