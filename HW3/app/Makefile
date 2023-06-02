#Makefile for a basic kernel module

all: app

app:
	arm-none-linux-gnueabi-gcc -static -o stopwatch_test stopwatch_test.c

clean:
	rm -rf stopwatch_test
