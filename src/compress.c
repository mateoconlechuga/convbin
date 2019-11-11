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
#include "ti8x.h"
#include "log.h"

#include "deps/zx7/zx7.h"

#include <string.h>

/* from decompress.asm */
/* run the makefile to print out the offset */
#define SIZE_OFFSET 41
extern unsigned char decompress_bin[];
extern unsigned int decompress_bin_len;

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

    if (delta > 0)
    {
        free(*arr);
        *arr = compressed;
    }

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
int compress_auto_8xp(unsigned char **arr, size_t *size)
{
    unsigned char *compressedarr = malloc(TI8X_MAXDATA_SIZE);
    unsigned char *newarr = malloc(TI8X_MAXDATA_SIZE);
    size_t origsize = *size;
    int ret = 0;

    memcpy(compressedarr, *arr + TI8X_ASMCOMP_LEN, *size);

    ret = compress_array(&compressedarr, size, COMPRESS_ZX7);
    if (ret == 0)
    {
        decompress_bin[SIZE_OFFSET + 0] = (origsize >> 0) & 0xff;
        decompress_bin[SIZE_OFFSET + 1] = (origsize >> 8) & 0xff;
        decompress_bin[SIZE_OFFSET + 2] = (origsize >> 16) & 0xff;

        memcpy(newarr, decompress_bin, (size_t)decompress_bin_len);
        memcpy(newarr + decompress_bin_len, compressedarr, *size);

        free(*arr);
        *arr = newarr;
        *size += decompress_bin_len;
    }

    return ret;
}
