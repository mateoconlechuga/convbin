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

#ifndef INPUT_H
#define INPUT_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "compress.h"

#define INPUT_MAX_NUM 16
#define INPUT_MAX_SIZE 0x80000

typedef enum
{
    IFORMAT_BIN,
    IFORMAT_TI8X_DATA,
    IFORMAT_TI8X_DATA_VAR,
    IFORMAT_CSV,
    IFORMAT_INVALID,
} iformat_t;

typedef struct
{
    const char *name;
    unsigned char arr[INPUT_MAX_SIZE];
    iformat_t format;
    compression_t compression;
    size_t size;
} input_file_t;

typedef struct
{
    input_file_t file[INPUT_MAX_NUM];
    iformat_t default_format;
    compression_t default_compression;
    unsigned int numfiles;
} input_t;

int input_read_file(input_file_t *file);

#ifdef	__cplusplus
}
#endif

#endif
