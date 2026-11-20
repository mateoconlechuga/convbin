# Copyright 2017-2026 Matt "MateoConLechuga" Waltz
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

BINUTILS_PREFIX ?= z80-none-elf-
AS := $(BINUTILS_PREFIX)as
LD := $(BINUTILS_PREFIX)ld
NM := $(BINUTILS_PREFIX)nm
OBJCOPY := $(BINUTILS_PREFIX)objcopy

BUILD_DIR := ../../obj/asm-helpers
HELPERS := zx7_decompressor zx0_decompressor extractor
ARRAYS := $(addsuffix .c,$(HELPERS))
HEADERS := $(addsuffix .h,$(HELPERS))

all: $(ARRAYS) $(HEADERS)

$(ARRAYS): %.c: $(BUILD_DIR)/% Makefile
	(cd $(BUILD_DIR) && xxd -i $*) > $@.tmp && mv $@.tmp $@

zx7_decompressor.h: PREFIX := DZX7
zx0_decompressor.h: PREFIX := DZX0
extractor.h: PREFIX := EXTRACTOR

$(HEADERS): %.h: $(BUILD_DIR)/%.sym header.awk Makefile
	awk -v prefix=$(PREFIX) -v name=$* -f header.awk $< > $@.tmp && mv $@.tmp $@

$(addprefix $(BUILD_DIR)/,$(HELPERS)): $(BUILD_DIR)/%: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary -j .bootstrap -j .relocated $< $@

$(HELPERS:%=$(BUILD_DIR)/%.sym): $(BUILD_DIR)/%.sym: $(BUILD_DIR)/%.elf
	LC_ALL=C $(NM) --extern-only --defined-only --format=posix --radix=d $< > $@.tmp && mv $@.tmp $@

$(HELPERS:%=$(BUILD_DIR)/%.elf): $(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.o helper.ld
	$(LD) -T helper.ld -o $@ $<

$(HELPERS:%=$(BUILD_DIR)/%.o): $(BUILD_DIR)/%.o: %.s helper.inc ti84pce.inc | $(BUILD_DIR)
	$(AS) -march=ez80+full -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.DELETE_ON_ERROR:
.PHONY: all clean
