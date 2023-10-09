INCLUDE_DIRS = -I/opt/intel/compilers_and_libraries_2020.0.166/linux/mpi/intel64/include/
INCLUDE_DIRS2 =
LIB_DIRS = -L/opt/intel/compilers_and_libraries_2020.0.166/linux/mpi/intel64/lib/debug -L/opt/intel/compilers_and_libraries_2020.0.166/linux/mpi/intel64/lib
MPICC = mpicc
CC = gcc

CDEFS=
CFLAGS= -g -Wall -fopenmp $(INCLUDE_DIRS2) $(CDEFS)
MPIFLAGS= -g -Wall $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT= seqsobel openmpsobel mpisobel

HFILES=
MPIFILES= mpisobel.c 
CFILES= seqsobel openmpsobel

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~
	-rm -f ${PRODUCT} ${DERIVED} ${GARBAGE}


seqsobel:	seqsobel.c
	$(CC) $(CFLAGS) -o $@ seqsobel.c $(LIBS) -lm

openmpsobel:	openmpsobel.c
	$(CC) $(CFLAGS) -o $@ openmpsobel.c $(LIBS) -lm
	
mpisobel:	mpisobel.c
	$(MPICC) $(MPIFLAGS) -o $@ mpisobel.c $(LIB_DIRS) -lm
