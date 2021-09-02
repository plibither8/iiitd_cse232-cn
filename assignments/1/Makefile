CC=gcc
CFLAGS=-I. -pthread
DEPS = 
OBJ = server.o client.o
%.o: %.c $(DEPS)
	$(CC) -o $@ $< $(CFLAGS)

all: $(OBJ)

clean:
	rm -f $(OBJ)

