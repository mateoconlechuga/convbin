#ifndef DZX7_H
#define DZX7_H

#define DZX7_DECOMPRESSOR_OFFSET 1
#define DZX7_COMPRESSED_DATA_ADDR_OFFSET 42
#define DZX7_INSERTMEM_SIZE_OFFSET 19
#define DZX7_INSERTMEM_ADDR_OFFSET 33
#define DZX7_DELMEM_CALL_OFFSET 57
#define DZX7_DELMEM_ADDR_OFFSET 50
#define DZX7_DELMEM_SIZE_OFFSET 54
#define DZX7_ASM_PRGM_SIZE_DELTA_OFFSET 62

extern unsigned char zx7_decompressor[];
extern unsigned int zx7_decompressor_len;

#endif
