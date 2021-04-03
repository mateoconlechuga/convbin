/*
 * Copyright 2017-2020 Matt "MateoConLechuga" Waltz
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

extern unsigned char decompress[];
extern unsigned int decompress_len;

static void reverse(unsigned char *first, unsigned char *last)
{
    unsigned char c;

    while (first < last)
    {
        c = *first;
        *first++ = *last;
        *last-- = c;
    }
}

static int compress_zx7(unsigned char **arr, size_t *size, long *delta)
{
    Optimal *opt;
    unsigned char *compressed;

    if (size == NULL || arr == NULL)
    {
        LL_DEBUG("invalid param in %s.", __func__);
        return 1;
    }

    opt = optimize(*arr, *size, 0);
    if (opt == NULL)
    {
        return 1;
    }

    compressed = compress(opt, *arr, *size, 0, size, delta);
    free(*arr);
    free(opt);

    *arr = compressed;
    if (compressed == NULL)
    {
        return 1;
    }

    return 0;
}

static int compress_zx7b(unsigned char **arr, size_t *size, long *delta)
{
    Optimal *opt;
    unsigned char *compressed;
    size_t newsize;
    size_t insize = *size;
    unsigned char *inarr = *arr;

    if (size == NULL || arr == NULL)
    {
        LL_DEBUG("invalid param in %s.", __func__);
        return 1;
    }

    reverse(inarr, inarr + insize - 1);

    opt = optimize(inarr, insize, 0);
    if (opt == NULL)
    {
        return 1;
    }

    compressed = compress(opt, *arr, insize, 0, &newsize, delta);
    free(opt);

    if (compressed == NULL)
    {
        return 1;
    }

    if (newsize + decompress_len + 1 >= insize)
    {
        reverse(inarr, inarr + insize - 1);
        free(compressed);
        return 2;
    }
    else
    {
        reverse(compressed, compressed + newsize - 1);
        free(*arr);
        *arr = compressed;
        *size = newsize;
    }

    return 0;
}

/*
 * Compress output array before writing to output.
 * Returns compressed array, or NULL if error.
 */
int compress_array(unsigned char **arr, size_t *size, long *delta, compression_t mode)
{
    switch (mode)
    {
        default:
            break;
        case COMPRESS_ZX7:
            return compress_zx7(arr, size, delta);
        case COMPRESS_ZX7B:
            return compress_zx7b(arr, size, delta);
    }

    return 1;
}

/*
 * Write 24-bit value into array.
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
#define DECOMPRESS_DELTA_SIZE_OFFSET 19
#define DECOMPRESS_DELTA_START_OFFSET 29
#define DECOMPRESS_PRGM_SIZE_OFFSET 37
#define DECOMPRESS_COMPRESSED_COPY_OFFSET 57
#define DECOMPRESS_COMPRESSED_END_OFFSET 45
#define DECOMPRESS_UNCOMPRESSED_END_OFFSET 49
#define DECOMPRESS_COMPRESSED_START_OFFSET 61
#define DECOMPRESS_UNCOMPRESSED_SIZE_OFFSET 66
#define DECOMPRESS_RESIZE_OFFSET 76
#define DECOMPRESS_RESIZE_SIZE_OFFSET 72

/*
 * Compress and create an auto-decompressing 8xp.
 */
int compress_auto_8xp(unsigned char **arr, size_t *size)
{
    unsigned char *compressedarr = malloc(INPUT_MAX_SIZE);
    unsigned char *newarr = malloc(INPUT_MAX_SIZE);
    unsigned char *inarr = *arr;
    unsigned int offset;
    size_t prgmsize;
    size_t uncompressedsize;
    size_t compressedsize;
    int ret = 0;
    long delta;

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
    else if (inarr[TI8X_MAGIC_C_OFFSET] == TI8X_MAGIC_C ||
             inarr[TI8X_MAGIC_ICE_OFFSET] == TI8X_MAGIC_ICE)
    {
        goto prepend_marker_only;
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
prepend_marker_only:
        offset++;
        if (offset >= *size)
        {
            LL_ERROR("Oddly formated 8x file.");
            return 1;
        }

        memcpy(newarr + TI8X_ASMCOMP_LEN, *arr + TI8X_ASMCOMP_LEN, offset);
    }

    uncompressedsize = *size - offset;
    prgmsize = uncompressedsize;

    memcpy(compressedarr, inarr + offset, uncompressedsize);

    ret = compress_array(&compressedarr, &prgmsize, &delta, COMPRESS_ZX7B);
    if (ret == 0)
    {
        unsigned int entryaddr;
        unsigned int deltasize;
        unsigned int deltastart;
        unsigned int compressedend;
        unsigned int uncompressedend;
        unsigned int prgmstart;
        unsigned int resizesize;
        unsigned int copyoffset;

        free(inarr);

        /* 512 byte buffer room just for kicks */
        resizesize = decompress_len + delta + 512;
        compressedsize = prgmsize + decompress_len;

        deltasize = (uncompressedsize - compressedsize) + resizesize;
        compress_write_word(decompress + DECOMPRESS_DELTA_SIZE_OFFSET, deltasize);

        prgmstart = (TI8X_USERMEM_ADDRESS - TI8X_ASMCOMP_LEN) + offset;
        compress_write_word(decompress + DECOMPRESS_COMPRESSED_START_OFFSET, prgmstart);

        deltastart = prgmstart + compressedsize;
        compress_write_word(decompress + DECOMPRESS_DELTA_START_OFFSET, deltastart);

        compressedend = deltastart - 1;
        compress_write_word(decompress + DECOMPRESS_COMPRESSED_END_OFFSET, compressedend);

        uncompressedend = prgmstart + uncompressedsize + resizesize;
        compress_write_word(decompress + DECOMPRESS_UNCOMPRESSED_END_OFFSET, uncompressedend);

        entryaddr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_write_word(decompress + DECOMPRESS_ENTRY_OFFSET, entryaddr);

        compress_write_word(decompress + DECOMPRESS_RESIZE_SIZE_OFFSET, resizesize);
        compress_write_word(decompress + DECOMPRESS_UNCOMPRESSED_SIZE_OFFSET, uncompressedsize);
        compress_write_word(decompress + DECOMPRESS_RESIZE_OFFSET, uncompressedend - resizesize);
        compress_write_word(decompress + DECOMPRESS_PRGM_SIZE_OFFSET, offset + uncompressedsize);

        copyoffset = (deltastart - compressedsize) + resizesize + 1;
        compress_write_word(decompress + DECOMPRESS_COMPRESSED_COPY_OFFSET, copyoffset);

        memcpy(newarr + offset, decompress, decompress_len);
        memcpy(newarr + offset + decompress_len, compressedarr, prgmsize);

        *arr = newarr;
        *size = prgmsize + decompress_len + offset;
    }
    else if (ret == 2)
    {
        free(compressedarr);
        free(newarr);
        ret = 0;
    }

    return ret;
}
