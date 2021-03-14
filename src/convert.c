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

#include "convert.h"
#include "extract.h"
#include "log.h"

#include <string.h>

/*
 * Builds a huge array from all the input files.
 * Compresses as needed.
 */
static int convert_build_data(input_t *input,
                              unsigned char **oarr,
                              size_t *osize,
                              size_t maxsize,
                              output_file_t *outfile,
                              compression_t compression)
{
    unsigned char *arr = malloc(INPUT_MAX_SIZE);
    size_t size = 0;
    unsigned int i;

    if (arr == NULL)
    {
        LL_DEBUG("memory error in %s.", __func__);
        return 1;
    }

    for (i = 0; i < input->numfiles; ++i)
    {
        input_file_t *file = &input->file[i];

        if (size + file->size > INPUT_MAX_SIZE)
        {
            LL_ERROR("input too large.");
            free(arr);
            return 1;
        }

        if (file->compression != COMPRESS_NONE)
        {
            unsigned char *comp_arr = malloc(file->size);
            long delta;
            int ret;

            if (comp_arr == NULL)
            {
                LL_DEBUG("memory error in %s.", __func__);
                return 1;
            }
            memcpy(comp_arr, file->arr, file->size);

            ret = compress_array(&comp_arr, &file->size, &delta, file->compression);
            if (ret != 0)
            {
                LL_ERROR("could not compress input.");
                free(arr);
                return ret;
            }

            memcpy(arr + size, comp_arr, file->size);
            free(comp_arr);
        } else {
            memcpy(arr + size, file->arr, file->size);
        }
        size += file->size;
    }

    if (compression != COMPRESS_NONE)
    {
        long delta;
        int ret;

        outfile->uncompressedsize = size;

        ret = compress_array(&arr, &size, &delta, compression);
        if (ret != 0)
        {
            LL_ERROR("could not compress output.");
            free(arr);
            return ret;
        }

        outfile->compressedsize = size;
    }

    if (size > maxsize)
    {
        LL_ERROR("input too large.");
        free(arr);
        return 1;
    }

    *oarr = arr;
    *osize = size;

    return 0;
}

/*
 * Builds TI 8x* format from array.
 */
static int convert_build_8x(unsigned char *arr, size_t size, output_file_t *outfile)
{
    unsigned char *ti8x;
    unsigned int checksum;
    size_t file_size;
    size_t data_size;
    size_t varb_size;
    size_t var_size;
    size_t name_size;

    if (size > outfile->var.maxsize)
    {
        LL_ERROR("input too large.");
        return 1;
    }

    name_size = strlen(outfile->var.name);
    file_size = size + TI8X_DATA + TI8X_CHECKSUM_LEN;
    data_size = size + TI8X_VAR_HEADER_LEN + TI8X_VARB_SIZE_LEN;
    var_size = size + TI8X_VARB_SIZE_LEN;
    varb_size = size;

    ti8x = outfile->arr;
    outfile->size = file_size;

    if (name_size > TI8X_VAR_NAME_LEN)
        name_size = TI8X_VAR_NAME_LEN;

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_NAME, outfile->var.name, name_size);
    memcpy(ti8x + TI8X_DATA, arr, size);

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
 * Converts data to TI 8x* format.
 */
static int convert_8x(input_t *input, output_file_t *outfile)
{
    unsigned char *arr = NULL;
    size_t size;
    int ret;

    ret = convert_build_data(input, &arr, &size,
                             INPUT_MAX_SIZE, outfile, outfile->compression);
    if (ret != 0)
    {
        return ret;
    }

    ret = convert_build_8x(arr, size, outfile);
    free(arr);

    return ret;
}

/*
 * Converts data to TI 8xp (program) format.
 */
static int convert_8xp(input_t *input, output_file_t *outfile)
{
    unsigned char *arr = NULL;
    size_t size;
    int ret;

    if (outfile->compression != COMPRESS_NONE)
    {
        LL_WARNING("ignoring compression mode");
    }

    ret = convert_build_data(input, &arr, &size,
                             INPUT_MAX_SIZE, outfile, COMPRESS_NONE);
    if (ret != 0)
    {
        return ret;
    }

    if (outfile->format == OFORMAT_8XP_AUTO_DECOMPRESS)
    {
        outfile->uncompressedsize = size;
        ret = compress_auto_8xp(&arr, &size);
        if (ret != 0)
        {
            LL_ERROR("could not compress.");
            return ret;
        }
        outfile->compressedsize = size;
    }

    if (size > outfile->var.maxsize)
    {
        unsigned int num_appvars = (size / outfile->var.maxsize) + 1;
        char appvar_names[10][10];
        unsigned int offset = TI8X_ASMCOMP_LEN;
        size_t tmpsize;
        size_t origsize;
        unsigned int i;
        char outname[4096];
        int pos = 0;

        memset(appvar_names, 0, sizeof appvar_names);
        memset(outname, 0, sizeof outname);

        strncpy(outname, outfile->name, sizeof outname - 10);
        strncat(outname, ".0.8xv", 10);

        pos = strlen(outname) - 5;

        if (num_appvars == 1)
        {
            LL_WARNING("input too large; split across 1 appvar...");
        }
        else if (num_appvars > 10)
        {
            LL_ERROR("input too large.");
            free(arr);
            return 1;
        }
        else
        {
            LL_WARNING("input too large; split across %u appvars...",
                num_appvars);
        }

        origsize = size;
        tmpsize = outfile->var.maxsize;

        for (i = 0; i < num_appvars; ++i)
        {
            size_t name_size = strlen(outfile->var.name);
            static output_file_t appvarfile;

            if (name_size > TI8X_VAR_NAME_LEN)
                name_size = TI8X_VAR_NAME_LEN;

            appvar_names[i][0] = TI8X_TYPE_APPVAR;
            memcpy(&appvar_names[i][1], outfile->var.name, name_size);
            appvar_names[i][name_size] = '0' + i;
            outname[pos] = '0' + i;

            appvarfile.name = outname;
            appvarfile.format = OFORMAT_8XV;
            appvarfile.var.maxsize = outfile->var.maxsize;
            memcpy(&appvarfile.var.name, &appvar_names[i][1], 9);
            appvarfile.var.type = TI8X_TYPE_APPVAR;
            appvarfile.var.archive = true;
            appvarfile.append = false;

            if (ret == 0)
            {
                ret = convert_build_8x(arr + offset, tmpsize, &appvarfile);
            }

            if (ret == 0)
            {
                ret = output_write_file(&appvarfile);
                if (ret != 0)
                {
                    break;
                }
            }

            LL_PRINT("[success] %s, %lu bytes.\n",
                appvarfile.name,
                (unsigned long)appvarfile.size);

            offset += tmpsize;

            /* handle last appvar */
            origsize -= tmpsize;
            if (origsize <= tmpsize)
            {
                tmpsize = origsize;
            }
        }

        /* write extractor program as final output */
        ret = extract_8xp_to_8xv(&arr, &size, appvar_names, num_appvars);
        if (ret == 0)
        {
            ret = convert_build_8x(arr, size, outfile);
        }
        free(arr);
    }
    else
    {
        ret = convert_build_8x(arr, size, outfile);
        free(arr);
    }

    return ret;
}

/*
 * Converts to an auto-extracting group file.
 */
int convert_auto_8xg(input_t *input, output_file_t *outfile)
{
    unsigned char *arr;
    unsigned char *ti8x;
    unsigned int checksum;
    size_t file_size;
    size_t data_size;
    size_t size;
    int ret;

    if (outfile->compression != COMPRESS_NONE)
    {
        LL_WARNING("ignoring compression mode");
    }

    ret = convert_build_data(input, &arr, &size,
                             outfile->var.maxsize, outfile, COMPRESS_NONE);
    if (ret != 0)
    {
        return ret;
    }

    file_size = size + TI8X_FILE_HEADER_LEN + TI8X_CHECKSUM_LEN;
    data_size = size;

    ti8x = outfile->arr;
    outfile->size = file_size;

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_VAR_HEADER, arr, size);

    free(arr);

    ti8x[TI8X_DATA_SIZE + 0] = (data_size >> 0) & 0xff;
    ti8x[TI8X_DATA_SIZE + 1] = (data_size >> 8) & 0xff;

    checksum = ti8x_checksum(ti8x, data_size);

    ti8x[file_size - 2] = (checksum >> 0) & 0xff;
    ti8x[file_size - 1] = (checksum >> 8) & 0xff;

    return 0;
}

/*
 * Converts input to raw binary (direct copy)
 */
int convert_bin(input_t *input, output_file_t *outfile)
{
    unsigned char *arr;
    size_t size;
    int ret;

    ret = convert_build_data(input, &arr, &size,
                             INPUT_MAX_SIZE, outfile, outfile->compression);
    if (ret != 0)
    {
        return ret;
    }

    memcpy(outfile->arr, arr, size);
    outfile->size = size;

    free(arr);

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
        ret = input_read_file(&input->file[i]);
        if (ret != 0)
        {
            return ret;
        }
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
        case OFORMAT_8XG:
            ret = convert_8x(input, &output->file);
            break;

        case OFORMAT_8XP:
        case OFORMAT_8XP_AUTO_DECOMPRESS:
            ret = convert_8xp(input, &output->file);
            break;

        case OFORMAT_8XG_AUTO_EXTRACT:
            ret = convert_auto_8xg(input, &output->file);
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
            float savings = (float)output->file.uncompressedsize -
                            (float)output->file.compressedsize;

            if (savings < 0.001)
            {
                savings = 0;
            }
            else
            {
                if (output->file.uncompressedsize != 0)
                {
                    savings /= (float)output->file.uncompressedsize;
                }
            }

            LL_PRINT("[success] %s, %lu bytes. (compressed %.2f%%)\n",
                output->file.name,
                (unsigned long)output->file.size,
                savings * 100.);
        }
        else
        {
            LL_PRINT("[success] %s, %lu bytes.\n",
                output->file.name,
                (unsigned long)output->file.size);
        }
    }

    return ret;
}
