CC=gcc
prog = keycount

all:
	$(CC) -g -W -Wall -lm -o $(prog) $(prog).c

clean:
	rm -f $(prog) *.o
