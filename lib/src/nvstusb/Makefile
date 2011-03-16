SRC = usb_libusb.c nvstusb.c
OBJ = $(SRC:.c=.o)
OUT = ../../libnvstusb.a

INCLUDES = -I/usr/include/libusb-1.0

CC = gcc
CFLAGS = -O2 -g $(INCLUDES)

all: $(OUT)

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(OUT)
