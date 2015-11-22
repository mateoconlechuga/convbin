CC = gcc
CFLAGS = -Wall -Wextra -fPIE -O3
LDFLAGS = -flto
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = ConvHex

.PHONY: ConvHex clean

all: ConvHex

ConvHex: $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)