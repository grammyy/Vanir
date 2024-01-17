CC = gcc
CFLAGS = -shared -O2 -mno-clwb
INCLUDE_DIR = -IC:\lua\include
LIB_DIR = -LC:\lua
LIBS = -llua54 -lgdi32

# Collect C files from the current directory and "modules" folder
SRC_FILES = $(wildcard *.c) $(wildcard modules/*.c)

# Output file
OUTPUT = vanir.dll

all: vanir

vanir: $(SRC_FILES)
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(SRC_FILES) -o $(OUTPUT) $(LIB_DIR) $(LIBS)
	@echo Compiled $(OUTPUT)

clean:
	del $(OUTPUT)

.PHONY: all clean