all:
	gcc -lusb-1.0 -o usbtest usbtest.c

clean:
	rm -f usbtest
