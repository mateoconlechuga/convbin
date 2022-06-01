#ifndef DZX0_H
#define DZX0_H

#define DZX0_DECOMPRESSOR_OFFSET 1
#define DZX0_COMPRESSED_DATA_ADDR_OFFSET 42
#define DZX0_INSERTMEM_SIZE_OFFSET 19
#define DZX0_INSERTMEM_ADDR_OFFSET 33
#define DZX0_DELMEM_CALL_OFFSET 57
#define DZX0_DELMEM_ADDR_OFFSET 50
#define DZX0_DELMEM_SIZE_OFFSET 54
#define DZX0_ASM_PRGM_SIZE_DELTA_OFFSET 62

extern unsigned char zx0_decompressor[];
extern unsigned int zx0_decompressor_len;

#endif
