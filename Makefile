# Copyright 2017-2020 Matt "MateoConLechuga" Waltz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

ifeq ($(OS),Windows_NT)
  TARGET ?= convbin.exe
  SHELL = cmd.exe
  NATIVEPATH = $(subst /,\,$1)
  RMDIR = ( rmdir /s /q $1 2>nul || call )
  MKDIR = ( mkdir $1 2>nul || call )
  STRIP = strip --strip-all "$1"
else
  TARGET ?= convbin
  NATIVEPATH = $(subst \,/,$1)
  MKDIR = mkdir -p $1
  RMDIR = rm -rf $1
  ifeq ($(shell uname -s),Darwin)
    STRIP = echo "no strip available"
  else
    STRIP = strip --strip-all "$1"
  endif
endif

V ?= 1
ifeq ($(V),1)
Q =
else
Q = @
endif

CC := gcc
CFLAGS := -Wall -Wextra -O3 -std=c89 -DNDEBUG -DLOG_BUILD_LEVEL=3 -flto
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
           $(SRCDIR)/extract.c \
           $(SRCDIR)/options.c \
           $(SRCDIR)/ti8x.c \
           $(SRCDIR)/log.c \
           $(SRCDIR)/asm/decompress.c \
           $(SRCDIR)/asm/extractor.c \
           $(DEPDIR)/zx7/compress.c \
           $(DEPDIR)/zx7/optimize.c

OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBRARIES :=

all: $(BINDIR)/$(TARGET)

release: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) $(LDFLAGS) $(call NATIVEPATH,$^) -o $(call NATIVEPATH,$@) $(addprefix -l, $(LIBRARIES))
	$(Q)$(call STRIP,$@)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(Q)$(call MKDIR,$(call NATIVEPATH,$(@D)))
	$(Q)$(CC) -c $(call NATIVEPATH,$<) $(CFLAGS) -o $(call NATIVEPATH,$@)

test:
	cd test && bash ./test.sh

clean:
	$(Q)$(call RMDIR,$(call NATIVEPATH,$(BINDIR)))
	$(Q)$(call RMDIR,$(call NATIVEPATH,$(OBJDIR)))

.PHONY: all release test clean
