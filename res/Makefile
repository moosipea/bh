CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c99

EXECUTABLE := asset_builder
ifeq ($(OS),Windows_NT)
	EXECUTABLE := asset_builder.exe
endif

HEADER := built_assets.h
SOURCE := built_assets.c
OBJECT := built_assets.o

$(OBJECT): $(SOURCE)
	$(CC) -c -o $@ $<

$(SOURCE): assets $(EXECUTABLE)
	./$(EXECUTABLE) $< $@ $(HEADER)

$(EXECUTABLE): asset_builder.o
	$(CC) -o $@ $^

RM := rm -f
ifeq ($(OS),Windows_NT)
	RM := del /F
endif

.PHONY: clean
clean:
	$(RM) $(EXECUTABLE)
	$(RM) $(EXECUTALBE).exe
	$(RM) $(HEADER)
	$(RM) $(wildcard *.o)
