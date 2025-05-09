CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ = main.o vinac.o lz.o
EXEC = vinac

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)

main.o: main.c vinac.h lz.h
	$(CC) $(CFLAGS) -c main.c

vinac.o: vinac.c vinac.h
	$(CC) $(CFLAGS) -c vinac.c

lz.o: lz.c lz.h
	$(CC) $(CFLAGS) -c lz.c

clean:
	rm -f *.o $(EXEC)