CC = gcc
CFLAGS = -g -O2 -Wall -I . 
LIB = -lpthread -lrt
# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.

echoserver: echoserveri.c csapp.o echo.o
	$(CC) $(CFLAGS) -o echoserver echoserveri.c csapp.o echo.o $(LIB)

csapp.o: ../csapp.c ../csapp.h
	$(CC) $(CFLAGS) -c ../csapp.c

echo.o: echo.c 
	$(CC) $(CFLAGS) -c echo.c

clean:
	rm -f *.o echo *~
	(cd cgi-bin; make clean)

