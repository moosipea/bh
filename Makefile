$(DEPENDENCIES) := external

GLFW_SOURCE_DIR := $(DEPENDENCIES)/glfw
GLFW_BUILD_DIR := $(GLFW_SOURCE_DIR)/$(OS)_build
GLFW_LIB := $(GLFW_BUILD_DIR)/src/libglfw3.a

CC := gcc
BINARY := main
CFLAGS := -Wall -Wextra -pedantic -ggdb \
	  -I$(GLFW_SOURCE_DIR)/include

LIB_DIRS := -L$(GLFW_BUILD_DIR)/src

LIBS := -lglfw3
ifeq ($(OS),Windows_NT)
	LIBS += -lgdi32
endif

$(BINARY): main.o
	$(CC) -o $@ $^ $(LIB_DIRS) $(LIBS)

.PHONY: dependencies
dependencies: $(GLFW_LIB)

$(GLFW_LIB):
	cmake -G "Unix Makefiles" \
	-S $(GLFW_SOURCE_DIR) -B $(GLFW_BUILD_DIR) \
	-D GLFW_BUILD_EXAMPLES=OFF \
	-D GLFW_BUILD_TESTS=OFF
	cmake --build $(GLFW_BUILD_DIR)

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
