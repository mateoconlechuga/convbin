/*
 * Copyright 2017-2021 Matt "MateoConLechuga" Waltz
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

static int convert_build_data(struct input *input,
                              uint8_t *data,
                              size_t *size,
                              size_t max_size,
                              struct output_file *output_file,
                              compression_t compression)
{
    size_t tmp_size = 0;
    unsigned int i;

    if (data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'.\n", __func__);
        return -1;
    }

    for (i = 0; i < input->nr_files; ++i)
    {
        struct input_file *file = &input->files[i];

        if (file->compression != COMPRESS_NONE)
        {
            long delta;
            int ret;

            ret = compress_array(file->data,
                &file->size, &delta, file->compression);
            if (ret != 0)
            {
                return ret;
            }
        }

        if (tmp_size + file->size >= INPUT_MAX_SIZE)
        {
            LOG_ERROR("Input too large.\n");
            return -1;
        }

        memcpy(data + tmp_size, file->data, file->size);
        tmp_size += file->size;
    }

    if (compression != COMPRESS_NONE)
    {
        long delta;
        int ret;

        output_file->uncompressed_size = tmp_size;

        ret = compress_array(data, &tmp_size, &delta, compression);
        if (ret != 0)
        {
            return ret;
        }

        output_file->compressed_size = tmp_size;
    }

    if (tmp_size > max_size)
    {
        LOG_ERROR("Input too large.\n");
        return -1;
    }

    *size = tmp_size;

    return 0;
}

static int convert_build_8x(uint8_t *data, size_t size, struct output_file *file)
{
    unsigned int checksum;
    size_t file_size;
    size_t data_size;
    size_t varb_size;
    size_t var_size;
    size_t name_size;
    uint8_t *ti8x;

    if (size > file->var.maxsize)
    {
        LOG_ERROR("Input too large.\n");
        return -1;
    }

    name_size = strlen(file->var.name);
    file_size = size + TI8X_DATA + TI8X_CHECKSUM_LEN;
    data_size = size + TI8X_VAR_HEADER_LEN + TI8X_VARB_SIZE_LEN;
    var_size = size + TI8X_VARB_SIZE_LEN;
    varb_size = size;

    ti8x = file->data;
    file->size = file_size;

    if (name_size > TI8X_VAR_NAME_LEN)
        name_size = TI8X_VAR_NAME_LEN;

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_NAME, file->var.name, name_size);
    memcpy(ti8x + TI8X_DATA, data, size);

    ti8x[TI8X_VAR_HEADER] = TI8X_MAGIC;
    ti8x[TI8X_TYPE] = file->var.type;
    ti8x[TI8X_ARCHIVE] = file->var.archive ? 0x80 : 0;

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

static int convert_8x(struct input *input, struct output_file *file)
{
    uint8_t data[INPUT_MAX_SIZE];
    size_t size;
    int ret;

    ret = convert_build_data(input, data, &size,
                             INPUT_MAX_SIZE, file, file->compression);
    if (ret != 0)
    {
        return ret;
    }

    ret = convert_build_8x(data, size, file);

    return ret;
}

static int convert_8xp(struct input *input, struct output_file *file)
{
    uint8_t data[INPUT_MAX_SIZE];
    size_t size;
    int ret;

    if (file->compression != COMPRESS_NONE)
    {
        LOG_WARNING("Ignoring compression mode!\n");
    }

    ret = convert_build_data(input, data, &size,
                             INPUT_MAX_SIZE, file, COMPRESS_NONE);
    if (ret != 0)
    {
        return ret;
    }

    if (file->format == OFORMAT_8XP_AUTO_DECOMPRESS)
    {
        file->uncompressed_size = size;

        ret = compress_auto_8xp(data, &size);
        if (ret != 0)
        {
            return ret;
        }

        file->compressed_size = size;
    }

    if (size > file->var.maxsize)
    {
        unsigned int num_appvars = (size / file->var.maxsize) + 1;
        char appvar_names[10][10];
        unsigned int offset = TI8X_ASMCOMP_LEN;
        size_t tmpsize;
        size_t origsize;
        unsigned int i;
        char outname[4096];
        int pos = 0;

        memset(appvar_names, 0, sizeof appvar_names);
        memset(outname, 0, sizeof outname);

        strncpy(outname, file->name, sizeof outname - 10);
        strncat(outname, ".0.8xv", 10);

        pos = strlen(outname) - 5;

        if (num_appvars == 1)
        {
            LOG_WARNING("Input too large; split across 1 appvar...\n");
        }
        else if (num_appvars > 10)
        {
            LOG_ERROR("Input too large.\n");
            return -1;
        }
        else
        {
            LOG_WARNING("Input too large; split across %u appvars...\n",
                num_appvars);
        }

        origsize = size;
        tmpsize = file->var.maxsize;

        for (i = 0; i < num_appvars; ++i)
        {
            size_t name_size = strlen(file->var.name);
            struct output_file appvarfile;

            if (name_size > TI8X_VAR_NAME_LEN)
                name_size = TI8X_VAR_NAME_LEN;

            appvar_names[i][0] = TI8X_TYPE_APPVAR;
            memcpy(&appvar_names[i][1], file->var.name, name_size);
            appvar_names[i][name_size] = '0' + i;
            outname[pos] = '0' + i;

            appvarfile.name = outname;
            appvarfile.format = OFORMAT_8XV;
            appvarfile.var.maxsize = file->var.maxsize;
            memcpy(&appvarfile.var.name, &appvar_names[i][1], 9);
            appvarfile.var.type = TI8X_TYPE_APPVAR;
            appvarfile.var.archive = true;
            appvarfile.append = false;

            ret = convert_build_8x(data + offset, tmpsize, &appvarfile);
            if (ret == 0)
            {
                ret = output_write_file(&appvarfile);
                if (ret != 0)
                {
                    break;
                }
            }

            LOG_PRINT("[success] %s, %lu bytes.\n",
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
        ret = extract_8xp_to_8xv(data, &size, appvar_names, num_appvars);
        if (ret == 0)
        {
            ret = convert_build_8x(data, size, file);
        }
    }
    else
    {
        ret = convert_build_8x(data, size, file);
    }

    return ret;
}

int convert_auto_8xg(struct input *input, struct output_file *file)
{
    uint8_t data[INPUT_MAX_SIZE];
    uint8_t *ti8x;
    unsigned int checksum;
    size_t file_size;
    size_t data_size;
    size_t size;
    int ret;

    if (file->compression != COMPRESS_NONE)
    {
        LOG_WARNING("Ignoring compression mode!\n");
    }

    ret = convert_build_data(input, data, &size,
                             file->var.maxsize, file, COMPRESS_NONE);
    if (ret != 0)
    {
        return ret;
    }

    file_size = size + TI8X_FILE_HEADER_LEN + TI8X_CHECKSUM_LEN;
    data_size = size;

    ti8x = file->data;
    file->size = file_size;

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_VAR_HEADER, data, size);

    ti8x[TI8X_DATA_SIZE + 0] = (data_size >> 0) & 0xff;
    ti8x[TI8X_DATA_SIZE + 1] = (data_size >> 8) & 0xff;

    checksum = ti8x_checksum(ti8x, data_size);

    ti8x[file_size - 2] = (checksum >> 0) & 0xff;
    ti8x[file_size - 1] = (checksum >> 8) & 0xff;

    return 0;
}

int convert_bin(struct input *input, struct output_file *file)
{
    return convert_build_data(input,
        file->data,
        &file->size,
        MAX_OUTPUT_SIZE,
        file,
        file->compression);
}

int convert_input_to_output(struct input *input, struct output *output)
{
    unsigned int i;
    int ret = 0;

    for (i = 0; i < input->nr_files; ++i)
    {
        ret = input_read_file(&input->files[i]);
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
            float savings = (float)output->file.uncompressed_size -
                            (float)output->file.compressed_size;

            if (savings < 0.001)
            {
                savings = 0;
            }
            else
            {
                if (output->file.uncompressed_size != 0)
                {
                    savings /= (float)output->file.uncompressed_size;
                }
            }

            LOG_PRINT("[success] %s, %lu bytes. (compressed %.2f%%)\n",
                output->file.name,
                (unsigned long)output->file.size,
                savings * 100.);
        }
        else
        {
            LOG_PRINT("[success] %s, %lu bytes.\n",
                output->file.name,
                (unsigned long)output->file.size);
        }
    }

    return ret;
}
