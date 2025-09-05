/*
 * Copyright 2017-2025 Matt "MateoConLechuga" Waltz
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

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "compress.h"
#include "ti8x.h"

#define MAX_OUTPUT_SIZE 0x40000
#define MAX_COMMENT_SIZE 42

typedef enum
{
    OFORMAT_C,
    OFORMAT_ASM,
    OFORMAT_BIN,
    OFORMAT_ICE,
    OFORMAT_8XP,
    OFORMAT_8XV,
    OFORMAT_8XG,
    OFORMAT_8XG_AUTO_EXTRACT,
    OFORMAT_8XP_COMPRESSED,
    OFORMAT_B84,
    OFORMAT_B83,
    OFORMAT_ZIP,
    OFORMAT_INVALID,
} oformat_t;

struct output_file
{
    const char *name;
    struct ti8x_var var;
    uint8_t data[MAX_OUTPUT_SIZE];
    char comment[MAX_COMMENT_SIZE];
    size_t size;
    size_t compressed_size;
    size_t uncompressed_size;
    compress_mode_t compression;
    compress_mode_t ti8xp_compression;
    oformat_t format;
    bool append;
    bool uppercase;
    bool compressed;
};

struct output
{
    struct output_file file;
};

void output_set_varname(struct output *output, const char *varname);

int output_write_file(const struct output_file *file);

#ifdef __cplusplus
}
#endif

#endif
