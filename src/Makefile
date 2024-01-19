CC = gcc
CFLAGS = -shared -O2 -mno-clwb
INCLUDE_DIR = -IC:\lua\include -IC:\SDL2-2.28.5\x86_64-w64-mingw32\include\SDL2
LIB_DIR = -LC:\lua -LC:\SDL2-2.28.5\x86_64-w64-mingw32\lib
LIBS = -lgdi32 -llua54 -lSDL2 -lopengl32

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