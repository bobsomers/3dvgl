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

CXX ?= g++
CXXFLAGS ?= -Wall -O2 -g
CPPFLAGS += $(INCLUDES)

all: $(OUT)

$(OUT): lib/libnvstusb.a $(OBJ)
	@echo ""
	@echo "============================================================"
	@echo "    Linking..."
	@echo "============================================================"
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
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
	$(CXX) $(CPPFLAGS) -c $(CXXFLAGS) -o $@ $<

clean:
	make -C lib clean
	rm -f $(OUT) $(OBJ) lib/libnvstusb.a
