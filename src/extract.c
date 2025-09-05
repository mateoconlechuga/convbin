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

#include "extract.h"
#include "input.h"
#include "ti8x.h"
#include "log.h"

#include "asm/extractor.h"

#include <string.h>

static void extract_write_word(uint8_t *addr, unsigned int value)
{
    addr[0] = (value >> 0) & 0xff;
    addr[1] = (value >> 8) & 0xff;
    addr[2] = (value >> 16) & 0xff;
}

int extract_8xp_to_8xv(uint8_t *data,
                       size_t *size,
                       char appvar_names[10][10],
                       unsigned int num_appvars)
{
    static uint8_t new_data[INPUT_MAX_SIZE];
    unsigned int extractorsize;
    unsigned int entryaddr;
    unsigned int offset;
    unsigned int i;
    size_t new_size;

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

        offset++;
        if (offset >= *size)
        {
odd_8x:
            LOG_ERROR("Oddly formated 8x file.\n");
            return -1;
        }

        memcpy(new_data + TI8X_ASMCOMP_LEN, data + TI8X_ASMCOMP_LEN, offset);
    }

    extract_write_word(extractor + EXTRACTOR_EXTRACT_SIZE_OFFSET, *size + 0x10);

    entryaddr = offset + TI8X_USERMEM_ADDRESS + 16;
    extract_write_word(extractor + EXTRACTOR_ENTRY_OFFSET, entryaddr);

    extractorsize = EXTRACTOR_APPVARS_OFFSET - 15 + num_appvars * TI8X_VARLOOKUP_LEN;
    extract_write_word(extractor + EXTRACTOR_PRGM_SIZE_OFFSET, extractorsize);

    memcpy(new_data + offset, extractor, extractor_len);
    offset += extractor_len;

    for (i = 0; i < num_appvars; ++i)
    {
        memcpy(new_data + offset, appvar_names[i], TI8X_VARLOOKUP_LEN);
        offset += TI8X_VARLOOKUP_LEN;
    }

    new_data[offset] = 0;
    offset++;

    new_size = offset;
    memcpy(data, new_data, new_size);
    *size = new_size;

    return 0;
}
