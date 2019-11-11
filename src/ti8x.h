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

#ifndef TI8X_H
#define TI8X_H

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TI8X_TYPE_UNKNOWN = -1,
    TI8X_TYPE_PRGM = 6,
    TI8X_TYPE_APPVAR = 21,
    TI8X_TYPE_GROUP = 21,
} ti8x_var_type_t;

typedef struct
{
    const char *name;
    bool archive;
    ti8x_var_type_t type;
    size_t maxsize;
} ti8x_var_t;

#define TI8X_CHECKSUM_LEN 2
#define TI8X_ASMCOMP_LEN 2
#define TI8X_VARB_SIZE_LEN 2
#define TI8X_VAR_HEADER_LEN 17

#define TI8X_FILE_HEADER 0x00
#define TI8X_DATA_SIZE 0x35
#define TI8X_VAR_HEADER 0x37
#define TI8X_VAR_SIZE0 0x39
#define TI8X_TYPE 0x3b
#define TI8X_NAME 0x3c
#define TI8X_ARCHIVE 0x45
#define TI8X_VAR_SIZE1 0x46
#define TI8X_VARB_SIZE 0x48
#define TI8X_DATA 0x4a

#define TI8X_MAGIC 0x0d
#define TI8X_MAX_FILE_SIZE (1024 * 66)
#define TI8X_MAXDATA_SIZE (0x10000 - 0x130)

#define TI8X_MINIMUM_MAXVAR_SIZE 4096
#define TI8X_DEFAULT_MAXVAR_SIZE TI8X_MAXDATA_SIZE

unsigned int ti8x_checksum(unsigned char *arr, size_t size);

#ifdef	__cplusplus
}
#endif

#endif
