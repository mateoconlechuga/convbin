all: decompress.c extractor.c

decompress.c: decompress.asm
	fasmg $< decompress
	xxd -i decompress > $@
	rm -f decompress

extractor.c: extractor.asm
	fasmg $< extractor
	xxd -i extractor > $@
	rm -f extractor

.PHONY: all clean

