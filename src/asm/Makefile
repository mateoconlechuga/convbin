all: decompress_bin.c

decompress_bin.c: decompress.asm
	fasmg $< decompress.bin
	xxd -i decompress.bin > $@
	rm -f decompress.bin

.PHONY: all clean
