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

#ifndef TI8X_H
#define TI8X_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TI8X_VAR_NAME_LEN 8
#define TI8X_VAR_MAX_NAME_LEN 512

typedef enum
{
    TI8X_TYPE_UNKNOWN = -1,
    TI8X_TYPE_PRGM = 6,
    TI8X_TYPE_APPVAR = 21,
    TI8X_TYPE_GROUP = 23,
} ti8x_var_type_t;

struct ti8x_var
{
    char name[TI8X_VAR_MAX_NAME_LEN + 1];
    unsigned int namelen;
    bool archive;
    ti8x_var_type_t type;
    size_t maxsize;
};

#define TI8X_CHECKSUM_LEN 2
#define TI8X_ASMCOMP_LEN 2
#define TI8X_VARB_SIZE_LEN 2
#define TI8X_VAR_HEADER_LEN 17
#define TI8X_FILE_HEADER_LEN 55

#define TI8X_FILE_HEADER 0x00
#define TI8X_COMMENT 0x0B
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

#define TI8X_USERMEM_ADDRESS 0xd1a881
#define TI8X_TOKEN_EXT 0xef
#define TI8X_TOKEN_ASM84CECMP 0x7b
#define TI8X_ICON_MAGIC 1
#define TI8X_DESCRIPTION_MAGIC 2
#define TI8X_MAGIC_JUMP 0xc3
#define TI8X_MAGIC_C 0
#define TI8X_MAGIC_ICE 0x7f
#define TI8X_MAGIC_C_OFFSET 2
#define TI8X_MAGIC_ICE_OFFSET 2
#define TI8X_MAGIC_JUMP_OFFSET_0 2
#define TI8X_MAGIC_JUMP_OFFSET_1 3

#define TI8X_VARLOOKUP_LEN 9

#define TI8X_TYPE_APPVAR 0x15

#define TI8EK_HEADER_LEN 71
#define TI8EK_SIGNATURE "**TIFL**"
#define TI8EK_SIGNATURE_LEN 8
#define TI8EK_VERSION 0x05

extern const unsigned char ti8x_file_header[11];

uint16_t ti8x_checksum(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
