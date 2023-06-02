CC=arm-none-linux-gnueabi-gcc
CFLAGS=-static -pthread

all: main.o

main.o: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f main.o