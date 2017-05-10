CC=gcc
CCOPTS=-std=gnu99 -Wall -g

EXE=burst
OBJS=burst.o

all: ${EXE}

burst: burst.o
	${CC} $^ -o $@

%.o : %.c
	${CC} ${CCOPTS} -c $<

clean:
	rm -f ${EXE} ${OBJS} ${LIBS}
