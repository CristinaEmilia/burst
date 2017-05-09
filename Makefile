all: burst

burst.o: burst.c
	gcc -c burst.c -o burst.o

burst: burst.o
	gcc burst.o -o burst

clean:
	-rm -f burst.o
	-rm -f burst
