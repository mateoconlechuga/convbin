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

#include "asm/zx7_decompressor.h"
#include "asm/zx0_decompressor.h"

#include <string.h>

#if 0
static void reverse(uint8_t *first, uint8_t *last)
{
    while (first < last)
    {
        uint8_t c = *first;
        *first++ = *last;
        *last-- = c;
    }
}
#endif

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

static void compress_zx0_progress(void)
{
    LOG_PRINT(".");
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

    LOG_PRINT("[compressing] [");

    compressed_data = zx0_compress(zx0_optimize(data, *size, 0, ZX0_MAX_OFFSET, compress_zx0_progress),
                                   data, *size, 0, 0, 1, &new_size, &deltai);
    LOG_PRINT("]\n");
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

int compress_array(uint8_t *data, size_t *size, long *delta, compression_t mode)
{
    switch (mode)
    {
        default:
            return -1;
        case COMPRESS_ZX7:
            return compress_zx7(data, size, delta);
        case COMPRESS_ZX0:
            return compress_zx0(data, size, delta);
    }
}

static void compress_wr24(uint8_t *addr, uint32_t value)
{
    addr[0] = (value >> 0) & 0xff;
    addr[1] = (value >> 8) & 0xff;
    addr[2] = (value >> 16) & 0xff;
}

static void compress_wr32(uint8_t *addr, uint32_t value)
{
    addr[0] = (value >> 0) & 0xff;
    addr[1] = (value >> 8) & 0xff;
    addr[2] = (value >> 16) & 0xff;
    addr[3] = (value >> 24) & 0xff;
}

int compress_8xp(uint8_t *data, size_t *size, compression_t mode)
{
    static uint8_t new_data[INPUT_MAX_SIZE];
    static uint8_t compressed_data[INPUT_MAX_SIZE];
    unsigned int offset;
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
            LOG_ERROR("Invalid 8x file.\n");
            return -1;
        }

        memcpy(new_data + TI8X_ASMCOMP_LEN, data + TI8X_ASMCOMP_LEN, offset);
    }

    uncompressed_size = *size - offset;
    memcpy(compressed_data, data + offset, uncompressed_size);

    compressed_size = uncompressed_size;
    ret = compress_array(compressed_data, &compressed_size, &delta, mode);
    if (ret < 0)
    {
        LOG_ERROR("Could not compress data.\n");
        return -1;
    }

    if ((mode == COMPRESS_ZX0 &&
        (compressed_size + zx0_decompressor_len) >= uncompressed_size) ||
        (mode == COMPRESS_ZX7 &&
        (compressed_size + zx7_decompressor_len) >= uncompressed_size))
    {
        return 1;
    }

    if (mode == COMPRESS_ZX7)
    {
        uint32_t decompressor_addr;
        uint32_t insertmem_addr;
        uint32_t compressed_data_addr;
        uint32_t delmem_addr;
        uint32_t delta_size;
        int32_t insertmem_size;
        int32_t delmem_size;
        int32_t asm_prgm_size_delta;
        uint32_t dzx_len = zx7_decompressor_len;
        uint8_t *dzx = zx7_decompressor;
        size_t new_size;

        /* add some extra space to just put my mind at ease */
        delta += 16;

        LOG_DEBUG("delta: %lu\n", delta);
        LOG_DEBUG("compressed_size: %zu\n", compressed_size);
        LOG_DEBUG("uncompressed_size: %zu\n", uncompressed_size);

        /* remve extra delta bytes from allocated decompressor */
        delta_size = dzx_len >= delta ? 0 : delta - dzx_len;
        LOG_DEBUG("delta_size: %u\n", delta_size);

        /* set the reloc address for the decompressor */
        decompressor_addr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_wr24(&dzx[DZX7_DECOMPRESSOR_OFFSET], decompressor_addr);
        LOG_DEBUG("decompressor_addr: $%06X\n", decompressor_addr);

        /* how many bytes needed for decompression */
        insertmem_size = (uncompressed_size - compressed_size) + delta_size;
        compress_wr24(&dzx[DZX7_INSERTMEM_SIZE_OFFSET], insertmem_size);
        LOG_DEBUG("insertmem_size: %u\n", insertmem_size);

        /* location to insert memory to */
        insertmem_addr = (TI8X_USERMEM_ADDRESS - TI8X_ASMCOMP_LEN) + offset;
        compress_wr24(&dzx[DZX7_INSERTMEM_ADDR_OFFSET], insertmem_addr);
        LOG_DEBUG("insertmem_addr: $%06X\n", insertmem_addr);

        /* location of compressed data due to shift by memory insertion */
        compressed_data_addr = insertmem_addr + insertmem_size + dzx_len;
        compress_wr24(&dzx[DZX7_COMPRESSED_DATA_ADDR_OFFSET], compressed_data_addr);
        LOG_DEBUG("compressed_data_addr: $%06X\n", compressed_data_addr);

        /* location to delmem memory from */
        delmem_addr = insertmem_addr + uncompressed_size;
        compress_wr24(&dzx[DZX7_DELMEM_ADDR_OFFSET], delmem_addr);
        LOG_DEBUG("delmem_addr: $%06X\n", delmem_addr);

        /* remove space for decompressor and delta at end */
        if (dzx_len >= delta)
        {
            delmem_size = 0;
            compress_wr32(&dzx[DZX7_DELMEM_CALL_OFFSET], 0);
            LOG_DEBUG("removed delmem\n");
        }
        else
        {
            delmem_size = delta - dzx_len;
            compress_wr24(&dzx[DZX7_DELMEM_SIZE_OFFSET], delmem_size);
            LOG_DEBUG("delmem_size: %u\n", delmem_size);
        }

        /* how many bytes to add to the resulting program size */
        asm_prgm_size_delta = (uncompressed_size - compressed_size);
        compress_wr24(&dzx[DZX7_ASM_PRGM_SIZE_DELTA_OFFSET], asm_prgm_size_delta);
        LOG_DEBUG("asm_prgm_size_delta: %u\n", asm_prgm_size_delta);

        /* write the decompressor + compressed data */
        memcpy(new_data + offset, dzx, dzx_len);
        memcpy(new_data + offset + dzx_len, compressed_data, compressed_size);

        new_size = compressed_size + dzx_len + offset;

        memcpy(data, new_data, new_size);
        *size = new_size;
    }

    if (mode == COMPRESS_ZX0)
    {
        uint32_t decompressor_addr;
        uint32_t insertmem_addr;
        uint32_t compressed_data_addr;
        uint32_t delmem_addr;
        uint32_t delta_size;
        int32_t insertmem_size;
        int32_t delmem_size;
        int32_t asm_prgm_size_delta;
        uint32_t dzx_len = zx0_decompressor_len;
        uint8_t *dzx = zx0_decompressor;
        size_t new_size;

        /* add some extra space to just put my mind at ease */
        delta += 16;

        LOG_DEBUG("delta: %lu\n", delta);
        LOG_DEBUG("compressed_size: %zu\n", compressed_size);
        LOG_DEBUG("uncompressed_size: %zu\n", uncompressed_size);

        /* remve extra delta bytes from allocated decompressor */
        delta_size = dzx_len >= delta ? 0 : delta - dzx_len;
        LOG_DEBUG("delta_size: %u\n", delta_size);

        /* set the reloc address for the decompressor */
        decompressor_addr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_wr24(&dzx[DZX0_DECOMPRESSOR_OFFSET], decompressor_addr);
        LOG_DEBUG("decompressor_addr: $%06X\n", decompressor_addr);

        /* how many bytes needed for decompression */
        insertmem_size = (uncompressed_size - compressed_size) + delta_size;
        compress_wr24(&dzx[DZX0_INSERTMEM_SIZE_OFFSET], insertmem_size);
        LOG_DEBUG("insertmem_size: %u\n", insertmem_size);

        /* location to insert memory to */
        insertmem_addr = (TI8X_USERMEM_ADDRESS - TI8X_ASMCOMP_LEN) + offset;
        compress_wr24(&dzx[DZX0_INSERTMEM_ADDR_OFFSET], insertmem_addr);
        LOG_DEBUG("insertmem_addr: $%06X\n", insertmem_addr);

        /* location of compressed data due to shift by memory insertion */
        compressed_data_addr = insertmem_addr + insertmem_size + dzx_len;
        compress_wr24(&dzx[DZX0_COMPRESSED_DATA_ADDR_OFFSET], compressed_data_addr);
        LOG_DEBUG("compressed_data_addr: $%06X\n", compressed_data_addr);

        /* location to delmem memory from */
        delmem_addr = insertmem_addr + uncompressed_size;
        compress_wr24(&dzx[DZX0_DELMEM_ADDR_OFFSET], delmem_addr);
        LOG_DEBUG("delmem_addr: $%06X\n", delmem_addr);

        /* remove space for decompressor and delta at end */
        if (dzx_len >= delta)
        {
            delmem_size = 0;
            compress_wr32(&dzx[DZX0_DELMEM_CALL_OFFSET], 0);
            LOG_DEBUG("removed delmem\n");
        }
        else
        {
            delmem_size = delta - dzx_len;
            compress_wr24(&dzx[DZX0_DELMEM_SIZE_OFFSET], delmem_size);
            LOG_DEBUG("delmem_size: %u\n", delmem_size);
        }

        /* how many bytes to add to the resulting program size */
        asm_prgm_size_delta = (uncompressed_size - compressed_size);
        compress_wr24(&dzx[DZX0_ASM_PRGM_SIZE_DELTA_OFFSET], asm_prgm_size_delta);
        LOG_DEBUG("asm_prgm_size_delta: %u\n", asm_prgm_size_delta);

        /* write the decompressor + compressed data */
        memcpy(new_data + offset, dzx, dzx_len);
        memcpy(new_data + offset + dzx_len, compressed_data, compressed_size);

        new_size = compressed_size + dzx_len + offset;

        memcpy(data, new_data, new_size);
        *size = new_size;
    }

    return ret;
}
