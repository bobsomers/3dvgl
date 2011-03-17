SRC = usb_libusb.c nvstusb.c
OBJ = $(SRC:.c=.o)
OUT = libnvstusb.a

CC = gcc
CFLAGS = -O2 -g

all: $(OUT)

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(OUT)
