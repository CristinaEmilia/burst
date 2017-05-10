EXE=burst
OBJS=burst.o

all: burst

burst: burst.c
	gcc burst.c -std=gnu99 -Wall -g -larchive -o burst
clean:
	rm -f ${EXE} ${OBJS}
