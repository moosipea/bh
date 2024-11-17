DEPENDENCIES_DIR := external

GLFW_SOURCE_DIR := $(DEPENDENCIES_DIR)/glfw
GLFW_BUILD_DIR := $(GLFW_SOURCE_DIR)/build
ifeq ($(OS),Windows_NT)
	GLFW_BUILD_DIR := $(GLFW_BUILD_DIR)-win32
endif
GLFW_LIB := $(GLFW_BUILD_DIR)/src/libglfw3.a

PYTHON := python3
ifeq ($(OS),Windows_NT)
	PYTHON := python
endif

GL_VERSION := gl:core=3.3
GLAD_SOURCE_DIR := $(DEPENDENCIES_DIR)/glad
GLAD_BUILD_DIR_NAME := build
GLAD_BUILD_DIR := $(GLAD_SOURCE_DIR)/$(GLAD_BUILD_DIR_NAME)
GLAD_LIB := $(GLAD_BUILD_DIR)/src/libglad.a

CC := gcc
BINARY := main
CFLAGS := -Wall -Wextra -pedantic -ggdb
	  
OBJECTS := main.o engine.o matrix.o

INCLUDES := -I$(GLFW_SOURCE_DIR)/include \
	    -I$(GLAD_BUILD_DIR)/include

LIB_DIRS := -L$(GLFW_BUILD_DIR)/src \
	    -L$(GLAD_BUILD_DIR)/src

LIBS := -lglfw3 -lm -pthread \
	-lglad 

ifeq ($(OS),Windows_NT)
	LIBS += -lgdi32
else
	LIBS += -ldl
endif

RM := rm -f
ifeq ($(OS),Windows_NT)
	RM := del /F
endif

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $^ $(LIB_DIRS) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $<

.PHONY: dependencies
dependencies: $(GLFW_LIB) $(GLAD_LIB)

$(GLFW_LIB):
	cmake -G "Unix Makefiles" \
	-S $(GLFW_SOURCE_DIR) -B $(GLFW_BUILD_DIR) \
	-D GLFW_BUILD_EXAMPLES=OFF \
	-D GLFW_BUILD_TESTS=OFF
	cmake --build $(GLFW_BUILD_DIR)

$(GLAD_LIB):
	cd $(GLAD_SOURCE_DIR) && $(PYTHON) -m glad --api $(GL_VERSION) --out-path=$(GLAD_BUILD_DIR_NAME)
	cd $(GLAD_BUILD_DIR)/src && $(CC) -c $(CFLAGS) -I../include gl.c
	cd $(GLAD_BUILD_DIR)/src && ar rcs libglad.a gl.o

.PHONY: clean
clean:
	$(RM) $(BINARY)
	$(RM) $(BINARY).exe
	$(RM) $(wildcard *.o)
