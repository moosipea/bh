CC := gcc
CFLAGS := -Wall -Wextra -pedantic

EXECUTABLE := asset_builder
ifeq ($(OS),Windows_NT)
	EXECUTABLE := asset_builder.exe
endif

HEADER := built_assets.h

$(HEADER): assets $(EXECUTABLE)
	./$(EXECUTABLE) $< $@

$(EXECUTABLE): asset_builder.o
	$(CC) -o $@ $^
