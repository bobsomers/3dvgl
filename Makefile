SRC = src/main.cpp src/scene.cpp src/screenshot.cpp
OBJ = $(SRC:.cpp=.o)
OUT = 3dvgl

INCLUDES = -Isrc \
		   -Ilib

LIBS = -Llib \
	   -lnvstusb \
	   -lGL \
	   -lGLU \
	   -lglut \
	   -lX11 \
	   -lXxf86vm \
	   -lpthread \
	   -lusb-1.0

CXX = g++
CFLAGS = -Wall -O2 -g $(INCLUDES)
LDFLAGS = $(LIBS) 

all: $(OUT)

$(OUT): lib/libnvstusb.a $(OBJ)
	@echo ""
	@echo "============================================================"
	@echo "    Linking..."
	@echo "============================================================"
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)
	@echo ""
	@echo "============================================================"
	@echo "    Done."
	@echo "============================================================"

lib/libnvstusb.a:
	@echo "============================================================"
	@echo "    Building libnvstusb from source..."
	@echo "============================================================"
	make -C lib
	@echo ""
	@echo "============================================================"
	@echo "    Building demo application..."
	@echo "============================================================"

.cpp.o:
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	make -C lib clean
	rm -f $(OUT) $(OBJ) lib/libnvstusb.a
