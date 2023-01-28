all:
	$(CC) -Wall -pedantic -Werror $(shell pkg-config --cflags libmicrohttpd libusb-1.0 ) -o temper-prom temper-prom.c temper.c $(shell pkg-config --libs libmicrohttpd libusb-1.0 )
