CC = gcc
CFLAGS = -Wall -Wextra -fPIE -O3
LDFLAGS = -flto
SOURCES = zx7/zx7.c zx7/zx7_opt.c main.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = convhex

.PHONY: convhex clean

all: convhex

convhex: $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)