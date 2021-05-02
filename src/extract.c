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

#include "extract.h"
#include "input.h"
#include "ti8x.h"
#include "log.h"

#include <string.h>

/*
 * Write 24-bit value into array.
 */
void extract_write_word(unsigned char *addr, unsigned int value)
{
    addr[0] = (value >> 0) & 0xff;
    addr[1] = (value >> 8) & 0xff;
    addr[2] = (value >> 16) & 0xff;
}

/* from extractor.asm */
/* run the asm makefile to print */
#define EXTRACTOR_ENTRY_OFFSET 1
#define EXTRACTOR_PRGM_SIZE_OFFSET 9
#define EXTRACTOR_EXTRACT_SIZE_OFFSET 19
#define EXTRACTOR_APPVARS_OFFSET 177

/*
 * Create and 8xp that extracts from 8xv.
 */
int extract_8xp_to_8xv(unsigned char **arr,
                       size_t *size,
                       char appvar_names[10][10],
                       unsigned int num_appvars)
{
    extern unsigned char extractor[];
    extern unsigned int extractor_len;
    unsigned char *newarr = malloc(TI8X_MAXDATA_SIZE);
    unsigned char *inarr = *arr;
    unsigned int extractorsize;
    unsigned int entryaddr;
    unsigned int offset;
    unsigned int i;
    int ret = 0;

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
                LL_ERROR("oddly formated 8x file.");
                return 1;
            }
        }

        offset++;
        if (offset >= *size)
        {
            LL_ERROR("oddly formated 8x file.");
            return 1;
        }

        memcpy(newarr + TI8X_ASMCOMP_LEN, *arr + TI8X_ASMCOMP_LEN, offset);
    }

    extract_write_word(extractor + EXTRACTOR_EXTRACT_SIZE_OFFSET, *size + 0x10);

    entryaddr = offset + TI8X_USERMEM_ADDRESS + 16;
    extract_write_word(extractor + EXTRACTOR_ENTRY_OFFSET, entryaddr);

    extractorsize = EXTRACTOR_APPVARS_OFFSET - 15 + num_appvars * TI8X_VARLOOKUP_LEN;
    extract_write_word(extractor + EXTRACTOR_PRGM_SIZE_OFFSET, extractorsize);

    memcpy(newarr + offset, extractor, extractor_len);
    offset += extractor_len;

    for (i = 0; i < num_appvars; ++i)
    {
        memcpy(newarr + offset, appvar_names[i], TI8X_VARLOOKUP_LEN);
        offset += TI8X_VARLOOKUP_LEN;
    }

    newarr[offset] = 0;
    offset++;

    free(*arr);
    *arr = NULL;

    *arr = newarr;
    *size = offset;

    return ret;
}
