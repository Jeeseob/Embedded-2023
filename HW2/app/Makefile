#Makefile for a basic kernel module

all: app

app:
	arm-none-linux-gnueabi-gcc -static -o test_dev_driver test_dev_driver.c

clean:
	rm -rf test_dev_driver
