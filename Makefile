# Build Notes
#
# FLAG			REQUIRES
# -DLINUX		-rdynamic
# -DTHREAD_SAFE		-D_REENTRANT -lpthread 
# -DDEMANGE_GNU_CXX	-liberty

    CXX = g++
    CCC = $(CXX)
     CC = gcc
     AS = /usr/bin/as
     OS = -DLINUX -rdynamic #-DSOLARIS
 CFLAGS = -g -I. -DDEMANGLE_GNU_CXX -DDO_STACK_TRACE \
	  $(OS) -DTHREAD_SAFE -D_REENTRANT # -DDEBUG
   LIBS = -L. -lmchk -lpthread -ldl -liberty

MCHKSRC = chk.c malloc.c lock.c list.c stacktrace.c free.c
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

stacktrace.o:	stacktrace.c
	$(CC) $(CFLAGS) -S stacktrace.c -o stacktrace.s
	@$(AS) -Qy -o stacktrace.o stacktrace.s
	@rm -f stacktrace.s

malloc.o:	malloc.c
	$(CC) $(CFLAGS) -S malloc.c -o malloc.s
	@$(AS) -Qy -o malloc.o malloc.s
	@rm -f malloc.s

lock.o:	lock.c
	$(CC) $(CFLAGS) -S lock.c -o lock.s
	@$(AS) -Qy -o lock.o lock.s
	@rm -f lock.s

free.o:	free.c
	$(CC) $(CFLAGS) -S free.c -o free.s
	@$(AS) -Qy -o free.o free.s
	@rm -f free.s

list.o:	list.c
	$(CC) $(CFLAGS) -S list.c -o list.s
	@$(AS) -Qy -o list.o list.s
	@rm -f list.s

chk.o:	chk.c chk.h
	$(CC) $(CFLAGS) -S chk.c -o chk.s
	@$(AS) -Qy -o chk.o chk.s
	@rm -f chk.s

clean:
	rm -f *.o *.s *.S_orig *~ *.a t? core



