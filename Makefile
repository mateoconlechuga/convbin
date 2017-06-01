CC = gcc
CFLAGS = -Wall -Wextra -fPIE -O3
LDFLAGS = -flto

ifeq ($(OS),Windows_NT)
SHELL = cmd.exe
RM = del /f 2>nul
EXECUTABLE = convhex.exe
SOURCES = zx7\zx7.c zx7\zx7_opt.c main.c
else
EXECUTABLE = convhex
RM = rm -f
SOURCES = zx7/zx7.c zx7/zx7_opt.c main.c
endif

OBJECTS := $(SOURCES:.c=.o)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(OBJECTS)

.PHONY: clean
