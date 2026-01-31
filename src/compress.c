/*
 * Copyright 2017-2026 Matt "MateoConLechuga" Waltz
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
#include <stdlib.h>

static int compress_zx7(uint8_t *data, size_t size, uint8_t **zx7_data, size_t *zx7_size, int32_t *delta)
{
    zx7_Optimal *opt;
    uint8_t *compressed_data;
    size_t new_size;
    long new_delta;

    if (data == NULL || zx7_data == NULL)
    {
        return -1;
    }

    opt = zx7_optimize(data, size, 0);
    if (opt == NULL)
    {
        LOG_ERROR("Could not optimize zx7.\n");
        return -1;
    }

    compressed_data = zx7_compress(opt, data, size, 0, &new_size, &new_delta);
    free(opt);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    *zx7_data = compressed_data;
    *zx7_size = new_size;
    *delta = new_delta;

    return 0;
}

static void compress_zx0_progress(void)
{
    LOG_PRINT(".");
}

static int compress_zx0(uint8_t *data, size_t size, uint8_t **zx0_data, size_t *zx0_size, int32_t *delta)
{
    zx0_BLOCK *optimal;
    uint8_t *compressed_data;
    int new_size;
    int new_delta;

    if (data == NULL || zx0_data == NULL || delta == NULL)
    {
        return -1;
    }

    LOG_PRINT("[info] Compressing [");

    optimal = zx0_optimize(data, size, 0, ZX0_MAX_OFFSET, compress_zx0_progress);
    if (optimal == NULL)
    {
        LOG_ERROR("Out of memory]\n");
        zx0_free();
        return -1;
    }

    compressed_data = zx0_compress(optimal, data, size, 0, 0, 1, &new_size, &new_delta);

    LOG_PRINT("]\n");

    if (compressed_data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        zx0_free();
        return -1;
    }

    *zx0_data = compressed_data;
    *zx0_size = new_size;
    *delta = new_delta;

    zx0_free();

    return 0;
}

static int compress_array_alloc(const uint8_t *data,
                                size_t size,
                                uint8_t **out_data,
                                size_t *out_size,
                                int32_t *delta,
                                compress_mode_t *mode)
{
    int ret = 0;

    if (data == NULL || out_data == NULL || out_size == NULL || delta == NULL || mode == NULL)
    {
        return -1;
    }

    *out_data = NULL;
    *out_size = size;

    switch (*mode)
    {
        case COMPRESS_NONE:
            *delta = 0;
            return 0;

        case COMPRESS_ZX7:
            ret = compress_zx7((uint8_t *)data, size, out_data, out_size, delta);
            break;

        case COMPRESS_ZX0:
            ret = compress_zx0((uint8_t *)data, size, out_data, out_size, delta);
            break;

        case COMPRESS_AUTO:
        {
            uint8_t *zx7_data = NULL;
            size_t zx7_size = 0;
            int32_t zx7_delta = 0;
            uint8_t *zx0_data = NULL;
            size_t zx0_size = 0;
            int32_t zx0_delta = 0;

            ret = compress_zx7((uint8_t *)data, size, &zx7_data, &zx7_size, &zx7_delta);
            if (ret)
            {
                return -1;
            }

            ret = compress_zx0((uint8_t *)data, size, &zx0_data, &zx0_size, &zx0_delta);
            if (ret)
            {
                free(zx7_data);
                return -1;
            }

            if (zx7_size <= zx0_size)
            {
                *out_data = zx7_data;
                *out_size = zx7_size;
                *delta = zx7_delta;
                *mode = COMPRESS_ZX7;
                free(zx0_data);
            }
            else
            {
                *out_data = zx0_data;
                *out_size = zx0_size;
                *delta = zx0_delta;
                *mode = COMPRESS_ZX0;
                free(zx7_data);
            }

            return 0;
        }

        default:
            ret = -1;
            break;
    }

    return ret;
}

int compress_array(uint8_t *data, size_t *size, int32_t *delta, compress_mode_t *mode)
{
    uint8_t *compressed_data = NULL;
    size_t compressed_size = 0;
    int ret = 0;

    if (data == NULL || size == NULL || delta == NULL || mode == NULL)
    {
        return -1;
    }

    ret = compress_array_alloc(data, *size, &compressed_data, &compressed_size, delta, mode);
    if (ret != 0)
    {
        return -1;
    }

    if (*mode == COMPRESS_NONE)
    {
        return 0;
    }

    if (compressed_data == NULL)
    {
        return -1;
    }

    memcpy(data, compressed_data, compressed_size);
    *size = compressed_size;

    free(compressed_data);

    return 0;
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

int compress_8xp(uint8_t *data, size_t *size, compress_mode_t mode)
{
    size_t uncompressed_size;
    size_t compressed_size;
    uint32_t offset;
    int32_t delta;
    int ret;
    uint8_t *compressed_data = NULL;

    if (data == NULL || size == NULL)
    {
        return -1;
    }

    if (mode == COMPRESS_NONE)
    {
        return 0;
    }

    offset = TI8X_ASMCOMP_LEN;

    /* handle icon and/or description to locate the data offset */
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
        uint32_t width = data[offset + 1];
        uint32_t height = data[offset + 2];

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

    }

    uncompressed_size = *size - offset;
    ret = compress_array_alloc(data + offset, uncompressed_size,
                               &compressed_data, &compressed_size, &delta, &mode);
    if (ret < 0)
    {
        LOG_ERROR("Could not compress data.\n");
        return -1;
    }

    if (compressed_data == NULL)
    {
        return -1;
    }

    if ((mode == COMPRESS_ZX0 &&
        (compressed_size + zx0_decompressor_len) >= uncompressed_size) ||
        (mode == COMPRESS_ZX7 &&
        (compressed_size + zx7_decompressor_len) >= uncompressed_size))
    {
        free(compressed_data);
        return 1;
    }

    data[0] = TI8X_TOKEN_EXT;
    data[1] = TI8X_TOKEN_ASM84CECMP;

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
        size_t dzx_len = zx7_decompressor_len;
        int32_t delta_adjusted;
        uint8_t *dzx = malloc(dzx_len);
        size_t new_size;

        if (dzx == NULL)
        {
            LOG_ERROR("Out of memory.\n");
            free(compressed_data);
            return -1;
        }

        memcpy(dzx, zx7_decompressor, dzx_len);

        /* add some extra space to just put my mind at ease */
        delta += 16;

        LOG_DEBUG("delta: %d\n", delta);
        LOG_DEBUG("compressed_size: %zu\n", compressed_size);
        LOG_DEBUG("uncompressed_size: %zu\n", uncompressed_size);

        /* remve extra delta bytes from allocated decompressor */
        delta_adjusted = delta;
        if (delta_adjusted <= 0 || (size_t)delta_adjusted <= dzx_len)
        {
            delta_size = 0;
        }
        else
        {
            delta_size = (uint32_t)(delta_adjusted - (int32_t)dzx_len);
        }
        LOG_DEBUG("delta_size: %u\n", delta_size);

        /* set the reloc address for the decompressor */
        decompressor_addr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_wr24(&dzx[DZX7_DECOMPRESSOR_OFFSET], decompressor_addr);
        LOG_DEBUG("decompressor_addr: $%06X\n", decompressor_addr);

        /* how many bytes needed for decompression */
        insertmem_size = (uncompressed_size - compressed_size) + delta_size;
        compress_wr24(&dzx[DZX7_INSERTMEM_SIZE_OFFSET], insertmem_size);
        LOG_DEBUG("insertmem_size: %d\n", insertmem_size);

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
        if (delta_adjusted <= 0 || (size_t)delta_adjusted <= dzx_len)
        {
            delmem_size = 0;
            compress_wr32(&dzx[DZX7_DELMEM_CALL_OFFSET], delmem_size);
            LOG_DEBUG("removed delmem\n");
        }
        else
        {
            delmem_size = delta_adjusted - (int32_t)dzx_len;
            compress_wr24(&dzx[DZX7_DELMEM_SIZE_OFFSET], delmem_size);
            LOG_DEBUG("delmem_size: %d\n", delmem_size);
        }

        /* how many bytes to add to the resulting program size */
        asm_prgm_size_delta = (uncompressed_size - compressed_size);
        compress_wr24(&dzx[DZX7_ASM_PRGM_SIZE_DELTA_OFFSET], asm_prgm_size_delta);
        LOG_DEBUG("asm_prgm_size_delta: %d\n", asm_prgm_size_delta);

        /* write the decompressor + compressed data */
        memcpy(data + offset, dzx, dzx_len);
        memcpy(data + offset + dzx_len, compressed_data, compressed_size);

        new_size = compressed_size + dzx_len + offset;

        *size = new_size;
        free(dzx);
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
        size_t dzx_len = zx0_decompressor_len;
        int32_t delta_adjusted;
        uint8_t *dzx = malloc(dzx_len);
        size_t new_size;

        if (dzx == NULL)
        {
            LOG_ERROR("Out of memory.\n");
            free(compressed_data);
            return -1;
        }

        memcpy(dzx, zx0_decompressor, dzx_len);

        /* add some extra space to just put my mind at ease */
        delta += 16;

        LOG_DEBUG("delta: %d\n", delta);
        LOG_DEBUG("compressed_size: %zu\n", compressed_size);
        LOG_DEBUG("uncompressed_size: %zu\n", uncompressed_size);

        /* remve extra delta bytes from allocated decompressor */
        delta_adjusted = delta;
        if (delta_adjusted <= 0 || (size_t)delta_adjusted <= dzx_len)
        {
            delta_size = 0;
        }
        else
        {
            delta_size = (uint32_t)(delta_adjusted - (int32_t)dzx_len);
        }
        LOG_DEBUG("delta_size: %u\n", delta_size);

        /* set the reloc address for the decompressor */
        decompressor_addr = TI8X_USERMEM_ADDRESS + offset + 16;
        compress_wr24(&dzx[DZX0_DECOMPRESSOR_OFFSET], decompressor_addr);
        LOG_DEBUG("decompressor_addr: $%06X\n", decompressor_addr);

        /* how many bytes needed for decompression */
        insertmem_size = (uncompressed_size - compressed_size) + delta_size;
        compress_wr24(&dzx[DZX0_INSERTMEM_SIZE_OFFSET], insertmem_size);
        LOG_DEBUG("insertmem_size: %d\n", insertmem_size);

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
        if (delta_adjusted <= 0 || (size_t)delta_adjusted <= dzx_len)
        {
            delmem_size = 0;
            compress_wr32(&dzx[DZX0_DELMEM_CALL_OFFSET], delmem_size);
            LOG_DEBUG("removed delmem\n");
        }
        else
        {
            delmem_size = delta_adjusted - (int32_t)dzx_len;
            compress_wr24(&dzx[DZX0_DELMEM_SIZE_OFFSET], delmem_size);
            LOG_DEBUG("delmem_size: %d\n", delmem_size);
        }

        /* how many bytes to add to the resulting program size */
        asm_prgm_size_delta = (uncompressed_size - compressed_size);
        compress_wr24(&dzx[DZX0_ASM_PRGM_SIZE_DELTA_OFFSET], asm_prgm_size_delta);
        LOG_DEBUG("asm_prgm_size_delta: %d\n", asm_prgm_size_delta);

        /* write the decompressor + compressed data */
        memcpy(data + offset, dzx, dzx_len);
        memcpy(data + offset + dzx_len, compressed_data, compressed_size);

        new_size = compressed_size + dzx_len + offset;

        *size = new_size;
        free(dzx);
    }

    free(compressed_data);

    return ret;
}
