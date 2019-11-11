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

#include "convert.h"
#include "log.h"

#include <string.h>

/*
 * Converts data to TI 8x* format.
 */
int convert_8x(input_t *input, output_file_t *outfile)
{
    unsigned char *arr = malloc(TI8X_MAXDATA_SIZE);
    unsigned char *ti8x;
    unsigned int checksum;
    unsigned int i;
    size_t file_size;
    size_t data_size;
    size_t varb_size;
    size_t var_size;
    size_t size;
    int ret;
    static const unsigned char ti8x_file_header[] =
        { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };

    if (input == NULL || outfile == NULL)
    {
        LL_DEBUG("invalid param in %s.", __func__);
        return 1;
    }

    size = 0;
    for (i = 0; i < input->numfiles; ++i)
    {
        input_file_t *file = &input->file[i];

        if (size + file->size > TI8X_MAXDATA_SIZE)
        {
            LL_ERROR("input too large to fit in output file.");
            return 1;
        }

        memcpy(arr + size, file->arr, file->size);
        size += file->size;
    }

    if (outfile->compression)
    {
        ret = compress_array(&arr, &size, outfile->compression);
        if (ret != 0)
        {
            LL_ERROR("could not compress.");
            return ret;
        }
    }

    if (outfile->format == OFORMAT_8XP_AUTO_DECOMPRESS)
    {
        ret = compress_auto_8xp(&arr, &size);
        if (ret != 0)
        {
            LL_ERROR("could not compress.");
            return ret;
        }
    }

    file_size = size + TI8X_DATA + TI8X_CHECKSUM_LEN;
    data_size = size + TI8X_VAR_HEADER_LEN + TI8X_VARB_SIZE_LEN;
    var_size = size + TI8X_VARB_SIZE_LEN;
    varb_size = size;

    ti8x = outfile->arr;
    outfile->size = file_size;

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_NAME, outfile->var.name, strlen(outfile->var.name));
    memcpy(ti8x + TI8X_DATA, arr, size);

    free(arr);

    ti8x[TI8X_VAR_HEADER] = TI8X_MAGIC;
    ti8x[TI8X_TYPE] = outfile->var.type;
    ti8x[TI8X_ARCHIVE] = outfile->var.archive ? 0x80 : 0;

    ti8x[TI8X_DATA_SIZE + 0] = (data_size >> 0) & 0xff;
    ti8x[TI8X_DATA_SIZE + 1] = (data_size >> 8) & 0xff;

    ti8x[TI8X_VARB_SIZE + 0] = (varb_size >> 0) & 0xff;
    ti8x[TI8X_VARB_SIZE + 1] = (varb_size >> 8) & 0xff;

    ti8x[TI8X_VAR_SIZE0 + 0] = (var_size >> 0) & 0xff;
    ti8x[TI8X_VAR_SIZE0 + 1] = (var_size >> 8) & 0xff;
    ti8x[TI8X_VAR_SIZE1 + 0] = (var_size >> 0) & 0xff;
    ti8x[TI8X_VAR_SIZE1 + 1] = (var_size >> 8) & 0xff;

    checksum = ti8x_checksum(ti8x, data_size);

    ti8x[TI8X_DATA + size + 0] = (checksum >> 0) & 0xff;
    ti8x[TI8X_DATA + size + 1] = (checksum >> 8) & 0xff;

    return 0;
}

/*
 * Converts to an auto-extracting group file.
 */
int convert_auto_8xg(input_t *input, output_file_t *outfile)
{
    unsigned char *arr = malloc(TI8X_MAXDATA_SIZE);
    unsigned char *ti8x;
    unsigned int checksum;
    unsigned int i;
    size_t file_size;
    size_t data_size;
    size_t varb_size;
    size_t var_size;
    size_t size;
    int ret;
    static const unsigned char ti8x_file_header[] =
        { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };

    if (input == NULL || outfile == NULL)
    {
        LL_DEBUG("invalid param in %s.", __func__);
        return 1;
    }

    size = 0;
    for (i = 0; i < input->numfiles; ++i)
    {
        input_file_t *file = &input->file[i];

        if (size + file->size > TI8X_MAXDATA_SIZE)
        {
            LL_ERROR("input too large to fit in output file.");
            return 1;
        }

        memcpy(arr + size, file->arr, file->size);
        size += file->size;
    }

    if (outfile->compression)
    {
        ret = compress_array(&arr, &size, outfile->compression);
        if (ret != 0)
        {
            LL_ERROR("could not compress.");
            return ret;
        }
    }

    if (outfile->format == OFORMAT_8XP_AUTO_DECOMPRESS)
    {
        ret = compress_auto_8xp(&arr, &size);
        if (ret != 0)
        {
            LL_ERROR("could not compress.");
            return ret;
        }
    }

    file_size = size + TI8X_DATA + TI8X_CHECKSUM_LEN;
    data_size = size + TI8X_VAR_HEADER_LEN + TI8X_VARB_SIZE_LEN;
    var_size = size + TI8X_VARB_SIZE_LEN;
    varb_size = size;

    ti8x = outfile->arr;
    outfile->size = file_size;

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_NAME, outfile->var.name, strlen(outfile->var.name));
    memcpy(ti8x + TI8X_DATA, arr, size);

    free(arr);

    ti8x[TI8X_VAR_HEADER] = TI8X_MAGIC;
    ti8x[TI8X_TYPE] = outfile->var.type;
    ti8x[TI8X_ARCHIVE] = outfile->var.archive ? 0x80 : 0;

    ti8x[TI8X_DATA_SIZE + 0] = (data_size >> 0) & 0xff;
    ti8x[TI8X_DATA_SIZE + 1] = (data_size >> 8) & 0xff;

    ti8x[TI8X_VARB_SIZE + 0] = (varb_size >> 0) & 0xff;
    ti8x[TI8X_VARB_SIZE + 1] = (varb_size >> 8) & 0xff;

    ti8x[TI8X_VAR_SIZE0 + 0] = (var_size >> 0) & 0xff;
    ti8x[TI8X_VAR_SIZE0 + 1] = (var_size >> 8) & 0xff;
    ti8x[TI8X_VAR_SIZE1 + 0] = (var_size >> 0) & 0xff;
    ti8x[TI8X_VAR_SIZE1 + 1] = (var_size >> 8) & 0xff;

    checksum = ti8x_checksum(ti8x, data_size);

    ti8x[TI8X_DATA + size + 0] = (checksum >> 0) & 0xff;
    ti8x[TI8X_DATA + size + 1] = (checksum >> 8) & 0xff;

    return 0;
}

/*
 * Converts input to raw binary (direct copy)
 */
int convert_bin(input_t *input, output_file_t *outfile)
{
    unsigned char *arr = malloc(MAX_OUTPUT_SIZE);
    unsigned int i;
    size_t size = 0;
    int ret = 0;

    for (i = 0; i < input->numfiles; ++i)
    {
        input_file_t *file = &input->file[i];

        if (size + file->size > MAX_OUTPUT_SIZE)
        {
            LL_ERROR("exceeded maximum data input.");
            return 1;
        }

        memcpy(arr + size, file->arr, file->size);
        size += file->size;
    }

    if (outfile->compression)
    {
        ret = compress_array(&arr, &size, outfile->compression);
    }

    memcpy(outfile->arr, arr, size);
    outfile->size = size;

    return ret;
}

/*
 * Convert file using options supplied via cli.
 * Returns 0 on success, otherwise nonzero.
 */
int convert_input_to_output(input_t *input, output_t *output)
{
    unsigned int i;
    int ret = 0;

    for (i = 0; i < input->numfiles; ++i)
    {
        input_read_file(&input->file[i]);
    }

    switch (output->file.format)
    {
        case OFORMAT_C:
        case OFORMAT_ASM:
        case OFORMAT_ICE:
        case OFORMAT_BIN:
            ret = convert_bin(input, &output->file);
            break;

        case OFORMAT_8XV:
        case OFORMAT_8XP:
        case OFORMAT_8XP_AUTO_DECOMPRESS:
            ret = convert_8x(input, &output->file);
            break;

        case OFORMAT_8XG:
            break;

        case OFORMAT_8XG_AUTO_EXTRACT:
            break;

        default:
            ret = 1;
            break;
    }

    if (ret == 0)
    {
        ret = output_write_file(&output->file);
    }

    if (ret == 0)
    {
        if (output->file.compression ||
            output->file.format == OFORMAT_8XP_AUTO_DECOMPRESS)
        {
            LL_PRINT("[success] %s, %lu bytes. (compressed)\n",
                output->file.name,
                output->file.size);
        }
        else
        {
            LL_PRINT("[success] %s, %lu bytes.\n",
                output->file.name,
                output->file.size);
        }
    }

    return ret;
}
