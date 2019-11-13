all: decompress.c

decompress.c: decompress.asm
	fasmg $< decompress
	xxd -i decompress > $@
	rm -f decompress

.PHONY: all clean
