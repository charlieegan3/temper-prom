all:
	gcc -Wall -pedantic -Werror -o temper-prom temper-prom.c temper.c /usr/lib/x86_64-linux-gnu/libusb-1.0.so -lmicrohttpd
