/*
 * Copyright 2017-2022 Matt "MateoConLechuga" Waltz
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

#include "deps/zx/zx7/zx7.h"
#include "deps/zx/zx0/zx0.h"
#include "asm/decompressor.h"

#include <string.h>

static void reverse(uint8_t *first, uint8_t *last)
{
    while (first < last)
    {
        uint8_t c = *first;
        *first++ = *last;
        *last-- = c;
    }
}

static int compress_zx7(uint8_t *data, size_t *size, long *delta)
{
    zx7_Optimal *opt;
    uint8_t *compressed_data;
    size_t new_size;

    if (size == NULL || data == NULL)
    {
        return -1;
    }

    opt = zx7_optimize(data, *size, 0);
    if (opt == NULL)
    {
        LOG_ERROR("Could not optimize zx7.\n");
        return -1;
    }

    compressed_data = zx7_compress(opt, data, *size, 0, &new_size, delta);
    free(opt);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Could not compress zx7.\n");
        return -1;
    }

    memcpy(data, compressed_data, new_size);
    *size = new_size;

    free(compressed_data);

    return 0;
}

static int compress_zx7b(uint8_t *data, size_t *size, long *delta)
{
    zx7_Optimal *opt;
    uint8_t *compressed_data;
    size_t new_size;

    if (size == NULL || data == NULL)
    {
        return -1;
    }

    reverse(data, data + *size - 1);

    opt = zx7_optimize(data, *size, 0);
    if (opt == NULL)
    {
        LOG_ERROR("Could not optimize zx7b.\n");
        return -1;
    }

    compressed_data = zx7_compress(opt, data, *size, 0, &new_size, delta);
    free(opt);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Could not compress zx7b.\n");
        return -1;
    }

    if (new_size + decompressor_len + 1 >= *size)
    {
        LOG_WARNING("Ignoring compression as it results in larger output.\n");
        reverse(data, data + *size - 1);
        free(compressed_data);
        return 1;
    }

    reverse(compressed_data, compressed_data + new_size - 1);

    memcpy(data, compressed_data, new_size);
    *size = new_size;

    free(compressed_data);

    return 0;
}


static int compress_zx0(uint8_t *data, size_t *size, long *delta)
{
    int deltai;
    uint8_t *compressed_data;
    int new_size;

    if (size == NULL || data == NULL)
    {
        return -1;
    }

    compressed_data = zx0_compress(zx0_optimize(data, *size, 0, ZX0_MAX_OFFSET), data, *size, 0, 1, 0, &new_size, &deltai);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Could not compress zx0.\n");
        return -1;
    }

    *delta = deltai;

    memcpy(data, compressed_data, new_size);
    *size = new_size;

    free(compressed_data);

    return 0;
}

static int compress_zx0b(uint8_t *data, size_t *size, long *delta)
{
    int deltai;
    uint8_t *compressed_data;
    int new_size;

    if (size == NULL || data == NULL)
    {
        return -1;
    }

    reverse(data, data + *size - 1);

    compressed_data = zx0_compress(zx0_optimize(data, *size, 0, ZX0_MAX_OFFSET), data, *size, 0, 1, 0, &new_size, &deltai);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Could not compress zx0b.\n");
        return -1;
    }

    *delta = deltai;

    if (new_size + decompressor_len + 1 >= *size)
    {
        LOG_WARNING("Ignoring compression as it results in larger output.\n");
        reverse(data, data + *size - 1);
        free(compressed_data);
        return 1;
    }

    reverse(compressed_data, compressed_data + new_size - 1);

    memcpy(data, compressed_data, new_size);
    *size = new_size;

    free(compressed_data);

    return 0;
}

int compress_array(uint8_t *data, size_t *size, long *delta, compression_t mode)
{
    switch (mode)
    {
        default:
            return -1;
        case COMPRESS_ZX7:
            return compress_zx7(data, size, delta);
        case COMPRESS_ZX7B:
            return compress_zx7b(data, size, delta);
        case COMPRESS_ZX0:
            return compress_zx0(data, size, delta);
        case COMPRESS_ZX0B:
            return compress_zx0b(data, size, delta);
    }
}

static void compress_write_word(uint8_t *addr, unsigned int value)
{
    addr[0] = (value >> 0) & 0xff;
    addr[1] = (value >> 8) & 0xff;
    addr[2] = (value >> 16) & 0xff;
}

int compress_auto_8xp(uint8_t *data, size_t *size, compression_t mode)
{
    uint8_t new_data[INPUT_MAX_SIZE];
    uint8_t compressed_data[INPUT_MAX_SIZE];
    unsigned int offset;
    size_t prgm_size;
    size_t uncompressed_size;
    size_t compressed_size;
    long delta;
    int ret;

    new_data[0] = TI8X_TOKEN_EXT;
    new_data[1] = TI8X_TOKEN_ASM84CECMP;

    offset = TI8X_ASMCOMP_LEN;

    /* handle icon and/or description by copying it if it exists */
    if (data[TI8X_MAGIC_JUMP_OFFSET_0] == TI8X_MAGIC_JUMP)
    {
        offset = TI8X_MAGIC_JUMP_OFFSET_0 + 4;
    }
    else if (data[TI8X_MAGIC_JUMP_OFFSET_1] == TI8X_MAGIC_JUMP)
    {
        offset = TI8X_MAGIC_JUMP_OFFSET_1 + 4;
    }
    else if (data[TI8X_MAGIC_C_OFFSET] == TI8X_MAGIC_C ||
             data[TI8X_MAGIC_ICE_OFFSET] == TI8X_MAGIC_ICE)
    {
        goto prepend_marker_only;
    }

    if (data[offset] == TI8X_ICON_MAGIC)
    {
        unsigned int width = data[offset + 1];
        unsigned int height = data[offset + 2];

        offset += 2 + width * height;
        goto move_to_end_of_description;
    }
    else if (data[offset] == TI8X_DESCRIPTION_MAGIC)
    {
move_to_end_of_description:
        offset += 1;
        while (data[offset])
        {
            offset++;
            if (offset >= *size)
            {
                goto odd_8x;
            }
        }
prepend_marker_only:
        offset++;
        if (offset >= *size)
        {
odd_8x:
            LOG_ERROR("Oddly formated 8x file.\n");
            return -1;
        }

        memcpy(new_data + TI8X_ASMCOMP_LEN, data + TI8X_ASMCOMP_LEN, offset);
    }

    uncompressed_size = *size - offset;
    prgm_size = uncompressed_size;

    memcpy(compressed_data, data + offset, prgm_size);

    ret = compress_array(compressed_data, &prgm_size, &delta, mode);
    if (ret < 0)
    {
        LOG_ERROR("Could not compress input.\n");
        return -1;
    }

    /* handle case where compressed > uncompressed */
    if (ret != 0)
    {
        return 1;
    }
    else
    {
        unsigned int entryaddr;
        unsigned int deltasize;
        unsigned int deltastart;
        unsigned int compressedend;
        unsigned int uncompressedend;
        unsigned int prgmstart;
        unsigned int resizesize;
        unsigned int copyoffset;
        size_t new_size;

        /* 512 byte buffer room just for kicks */
        resizesize = decompressor_len + delta + 512;
        compressed_size = prgm_size + decompressor_len;

        deltasize = (uncompressed_size - compressed_size) + resizesize;
        compress_write_word(decompressor + DECOMPRESSOR_DELTA_SIZE_OFFSET, deltasize);

        prgmstart = (TI8X_USERMEM_ADDRESS - TI8X_ASMCOMP_LEN) + offset;
        compress_write_word(decompressor + DECOMPRESSOR_COMPRESSED_START_OFFSET, prgmstart);

        deltastart = prgmstart + compressed_size;
        compress_write_word(decompressor + DECOMPRESSOR_DELTA_START_OFFSET, deltastart);

        compressedend = deltastart - 1;
        compress_write_word(decompressor + DECOMPRESSOR_COMPRESSED_END_OFFSET, compressedend);

        uncompressedend = prgmstart + uncompressed_size + resizesize;
        compress_write_word(decompressor + DECOMPRESSOR_UNCOMPRESSED_END_OFFSET, uncompressedend);

        entryaddr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_write_word(decompressor + DECOMPRESSOR_ENTRY_OFFSET, entryaddr);

        compress_write_word(decompressor + DECOMPRESSOR_RESIZE_SIZE_OFFSET, resizesize);
        compress_write_word(decompressor + DECOMPRESSOR_UNCOMPRESSED_SIZE_OFFSET, uncompressed_size);
        compress_write_word(decompressor + DECOMPRESSOR_RESIZE_OFFSET, uncompressedend - resizesize);
        compress_write_word(decompressor + DECOMPRESSOR_PRGM_SIZE_OFFSET, offset + uncompressed_size);

        copyoffset = (deltastart - compressed_size) + resizesize + 1;
        compress_write_word(decompressor + DECOMPRESSOR_COMPRESSED_COPY_OFFSET, copyoffset);

        memcpy(new_data + offset, decompressor, decompressor_len);
        memcpy(new_data + offset + decompressor_len, compressed_data, prgm_size);

        new_size = prgm_size + decompressor_len + offset;

        memcpy(data, new_data, new_size);
        *size = new_size;
    }

    return ret;
}
