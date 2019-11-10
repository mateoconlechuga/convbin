all: unsplit.asm decompress.asm
	fasmg decompress.asm decompress.bin
	fasmg unsplit.asm unsplit.bin
	xxd -i decompress.bin > decompress_bin.c
	xxd -i unsplit.bin > unsplit_bin.c

clean:
	rm -f unsplit.bin decompress.bin unsplit_bin.c decompress_bin.c
