/*
 * Copyright 2017-2019 Matt "MateoConLechuga" Waltz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "compress.h"
#include "input.h"
#include "ti8x.h"
#include "log.h"

#include "deps/zx7/zx7.h"

#include <string.h>

static int compress_zx7(unsigned char **arr, size_t *size)
{
    long delta;
    Optimal *opt;
    unsigned char *compressed;

    if (size == NULL || arr == NULL)
    {
        LL_DEBUG("invalid param in %s.", __func__);
        return 1;
    }

    opt = optimize(*arr, *size);
    compressed = compress(opt, *arr, *size, size, &delta);
    free(*arr);
    *arr = compressed;

    free(opt);
    return 0;
}

/*
 * Compress output array before writing to output.
 * Returns compressed array, or NULL if error.
 */
int compress_array(unsigned char **arr, size_t *size, compression_t mode)
{
    /* zx7 is only compression mode */
    (void)mode;

    return compress_zx7(arr, size);
}

/*
 * Compress and create an auto-decompressing 8xp.
 */
void compress_write_word(unsigned char *addr, unsigned int value)
{
    addr[0] = (value >> 0) & 0xff;
    addr[1] = (value >> 8) & 0xff;
    addr[2] = (value >> 16) & 0xff;
}

/* from decompress.asm */
/* run the asm makefile to print */
#define DECOMPRESS_ENTRY_OFFSET 1
#define DECOMPRESS_DATA_OFFSET 102
#define DECOMPRESS_COMPRESSED_SIZE_OFFSET 19
#define DECOMPRESS_UNCOMPRESSED_SIZE_OFFSET 55
#define DECOMPRESS_USERMEM_OFFSET_0 23
#define DECOMPRESS_USERMEM_OFFSET_1 74
#define DECOMPRESS_USERMEM_OFFSET_2 107

/*
 * Compress and create an auto-decompressing 8xp.
 */
int compress_auto_8xp(unsigned char **arr, size_t *size)
{
    extern unsigned char decompress[];
    extern unsigned int decompress_len;
    unsigned char *compressedarr = malloc(INPUT_MAX_SIZE);
    unsigned char *newarr = malloc(TI8X_MAXDATA_SIZE);
    unsigned char *inarr = *arr;
    unsigned int offset;
    size_t compressedsize;
    size_t uncompressedsize;
    int ret = 0;

    newarr[0] = TI8X_TOKEN_EXT;
    newarr[1] = TI8X_TOKEN_ASM84CECMP;

    offset = TI8X_ASMCOMP_LEN;

    /* handle icon and/or description by copying it if it exists */
    if (inarr[TI8X_MAGIC_JUMP_OFFSET_0] == TI8X_MAGIC_JUMP)
    {
        offset = TI8X_MAGIC_JUMP_OFFSET_0 + 4;
    }
    else if (inarr[TI8X_MAGIC_JUMP_OFFSET_1] == TI8X_MAGIC_JUMP)
    {
        offset = TI8X_MAGIC_JUMP_OFFSET_1 + 4;
    }

    if (inarr[offset] == TI8X_ICON_MAGIC)
    {
        unsigned char width = (*arr)[offset + 1];
        unsigned char height = (*arr)[offset + 2];

        offset += 2 + width * height;
        goto move_to_end_of_description;
    }
    else if (inarr[offset] == TI8X_DESCRIPTION_MAGIC)
    {
move_to_end_of_description:
        offset += 1;
        while (inarr[offset])
        {
            offset++;
            if (offset >= *size)
            {
                LL_ERROR("Oddly formated 8x file.");
                return 1;
            }
        }

        offset++;
        if (offset >= *size)
        {
            LL_ERROR("Oddly formated 8x file.");
            return 1;
        }

        memcpy(newarr + TI8X_ASMCOMP_LEN, *arr + TI8X_ASMCOMP_LEN, offset);
    }

    uncompressedsize = *size - offset;
    compressedsize = uncompressedsize;

    memcpy(compressedarr, inarr + offset, uncompressedsize);
    free(inarr);

    ret = compress_array(&compressedarr, &compressedsize, COMPRESS_ZX7);
    if (ret == 0)
    {
        unsigned int entryaddr;
        unsigned int dataoffset;
        unsigned int usermemoffset;

        compress_write_word(decompress + DECOMPRESS_COMPRESSED_SIZE_OFFSET, compressedsize + decompress_len);
        compress_write_word(decompress + DECOMPRESS_UNCOMPRESSED_SIZE_OFFSET, uncompressedsize);

        entryaddr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_write_word(decompress + DECOMPRESS_ENTRY_OFFSET, entryaddr);

        dataoffset = offset + decompress_len + TI8X_VARB_SIZE_LEN;
        compress_write_word(decompress + DECOMPRESS_DATA_OFFSET, dataoffset);

        usermemoffset = TI8X_USERMEM_ADDRESS - TI8X_ASMCOMP_LEN + offset;
        compress_write_word(decompress + DECOMPRESS_USERMEM_OFFSET_0, usermemoffset);
        compress_write_word(decompress + DECOMPRESS_USERMEM_OFFSET_1, usermemoffset);
        compress_write_word(decompress + DECOMPRESS_USERMEM_OFFSET_2, usermemoffset);

        memcpy(newarr + offset, decompress, decompress_len);
        memcpy(newarr + offset + decompress_len, compressedarr, compressedsize);

        *arr = newarr;
        *size = compressedsize + decompress_len + offset;
    }

    return ret;
}
