CC = gcc
CFLAGS = -W -Wall -Wextra -std=c89 -fPIE -O3 -flto
LDFLAGS = -flto

ifeq ($(OS),Windows_NT)
TARGET := convhex.exe
SHELL = cmd.exe
RM = del /f 2>nul
SOURCES = zx7\zx7.c zx7\zx7_opt.c main.c
else
TARGET := convhex
RM = rm -f
SOURCES = zx7/zx7.c zx7/zx7_opt.c main.c
endif

OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET) $(OBJECTS)

.PHONY: clean
