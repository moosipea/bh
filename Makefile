$(DEPENDENCIES) := external

GLFW_SOURCE_DIR := $(DEPENDENCIES)/glfw
GLFW_BUILD_DIR := $(GLFW_SOURCE_DIR)/$(OS)_build
GLFW_LIB := $(GLFW_BUILD_DIR)/src/libglfw3.a

GL_VERSION := gl:core=3.3
GLAD_SOURCE_DIR := $(DEPENDENCIES)/glad
GLAD_BUILD_DIR := $(GLAD_SOURCE_DIR)/build
GLAD_LIB := $(GLAD_BUILD_DIR)/src/libglad.a

CC := gcc
BINARY := main
CFLAGS := -Wall -Wextra -pedantic -ggdb
	  
INCLUDES := -I$(GLFW_SOURCE_DIR)/include \
	    -I$(GLAD_BUILD_DIR)/include
LIB_DIRS := -L$(GLFW_BUILD_DIR)/src

LIBS := -lglfw3
ifeq ($(OS),Windows_NT)
	LIBS += -lgdi32
endif

$(BINARY): main.o
	$(CC) -o $@ $^ $(LIB_DIRS) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $<

.PHONY: dependencies
dependencies: $(GLFW_LIB) $(GLAD_OBJECT)

$(GLFW_LIB):
	cmake -G "Unix Makefiles" \
	-S $(GLFW_SOURCE_DIR) -B $(GLFW_BUILD_DIR) \
	-D GLFW_BUILD_EXAMPLES=OFF \
	-D GLFW_BUILD_TESTS=OFF
	cmake --build $(GLFW_BUILD_DIR)

$(GLAD_LIB):
	cd $(GLAD_SOURCE_DIR) && python3 -m glad --api $(GL_VERSION) --out-path=build
	cd $(GLAD_BUILD_DIR)/src && $(CC) -c $(CFLAGS) -I../include gl.c
	cd $(GLAD_BUILD_DIR)/src && ar rcs libglad.a gl.o

ifeq ($(OS),Windows_NT)
	RM := del /F
else
	RM := rm -f
endif

.PHONY: clean
clean:
	$(RM) $(BINARY)
	$(RM) $(BINARY).exe
	$(RM) $(wildcard *.o)
