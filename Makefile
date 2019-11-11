ifeq ($(OS),Windows_NT)
  TARGET ?= convbin.exe
  SHELL = cmd.exe
  NATIVEPATH = $(subst /,\,$1)
  MKDIR = if not exist "$1" mkdir "$1"
  RMDIR = del /f "$1" 2>nul
  STRIP = strip --strip-all "$1"
else
  TARGET ?= convbin
  NATIVEPATH = $(subst \,/,$1)
  MKDIR = mkdir -p "$1"
  RMDIR = rm -rf "$1"
  ifeq ($(shell uname -s),Darwin)
    STRIP = echo "no strip available"
  else
    STRIP = strip --strip-all "$1"
  endif
endif

CC := gcc
CFLAGS := -Wall -Wextra -O3 -DNDEBUG -DLOG_BUILD_LEVEL=3 -std=c89
LDFLAGS := -flto

BINDIR := ./bin
OBJDIR := ./obj
SRCDIR := ./src
DEPDIR := ./src/deps
SOURCES := $(SRCDIR)/main.c \
           $(SRCDIR)/convert.c \
           $(SRCDIR)/input.c \
           $(SRCDIR)/output.c \
           $(SRCDIR)/compress.c \
           $(SRCDIR)/options.c \
           $(SRCDIR)/ti8x.c \
           $(SRCDIR)/log.c \
           $(SRCDIR)/asm/decompress_bin.c \
           $(DEPDIR)/zx7/compress.c \
           $(DEPDIR)/zx7/optimize.c

OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBRARIES :=

all: $(BINDIR)/$(TARGET)

release: $(BINDIR)/$(TARGET)
	$(call STRIP,$^)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(CC) $(LDFLAGS) $(call NATIVEPATH,$^) -o $(call NATIVEPATH,$@) $(addprefix -l, $(LIBRARIES))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS) -o $(call NATIVEPATH,$@)

clean:
	$(call RMDIR,$(call NATIVEPATH,$(BINDIR)))
	$(call RMDIR,$(call NATIVEPATH,$(OBJDIR)))

.PHONY: all release clean
