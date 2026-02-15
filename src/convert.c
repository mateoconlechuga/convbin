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

#include "convert.h"
#include "extract.h"
#include "log.h"
#include "deps/miniz/miniz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int convert_build_data(struct input *input,
                              uint8_t *data,
                              size_t *size,
                              size_t max_size,
                              struct output_file *output_file,
                              compress_mode_t compression)
{
    size_t tmp_size = 0;
    uint32_t i;

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
            int32_t delta;
            int ret;

            ret = compress_array(file->data,
                &file->size, &delta, &file->compression);
            if (ret < 0)
            {
                return ret;
            }
        }

        if (tmp_size > max_size || file->size > max_size - tmp_size)
        {
            LOG_ERROR("Input too large.\n");
            return -1;
        }

        memcpy(data + tmp_size, file->data, file->size);
        tmp_size += file->size;
    }

    if (compression != COMPRESS_NONE)
    {
        int32_t delta;
        int ret;

        output_file->uncompressed_size = tmp_size;

        ret = compress_array(data, &tmp_size, &delta, &compression);
        if (ret < 0)
        {
            return ret;
        }

        output_file->compressed = ret == 0;
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

static int convert_total_input_size(const struct input *input, size_t *total_size)
{
    size_t total = 0;
    uint32_t i;

    if (input == NULL || total_size == NULL)
    {
        return -1;
    }

    for (i = 0; i < input->nr_files; ++i)
    {
        if (total > SIZE_MAX - input->files[i].size)
        {
            LOG_ERROR("Input too large.\n");
            return -1;
        }
        total += input->files[i].size;
    }

    *total_size = total;
    return 0;
}

static int convert_alloc_input_buffer(const struct input *input,
                                      uint8_t **data,
                                      size_t *capacity)
{
    size_t total_size = 0;
    size_t alloc_size;
    uint8_t *buffer;

    if (input == NULL || data == NULL)
    {
        return -1;
    }

    if (convert_total_input_size(input, &total_size) != 0)
    {
        return -1;
    }

    alloc_size = total_size == 0 ? 1 : total_size;
    buffer = malloc(alloc_size);
    if (buffer == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    *data = buffer;
    if (capacity != NULL)
    {
        *capacity = total_size;
    }
    return 0;
}

static int convert_build_8x(uint8_t *data, size_t size, struct output_file *file)
{
    uint16_t checksum;
    size_t file_size;
    size_t data_size;
    size_t varb_size;
    size_t var_size;
    uint8_t *ti8x;

    if (size > file->var.maxsize)
    {
        LOG_ERROR("Input too large.\n");
        return -1;
    }

    file_size = size + TI8X_DATA + TI8X_CHECKSUM_LEN;
    data_size = size + TI8X_VAR_HEADER_LEN + TI8X_VARB_SIZE_LEN;
    var_size = size + TI8X_VARB_SIZE_LEN;
    varb_size = size;

    if (output_reserve_data(file, file_size) != 0)
    {
        return -1;
    }

    ti8x = file->data;
    file->size = file_size;
    memset(ti8x, 0, file_size);

    memcpy(ti8x + TI8X_COMMENT, file->comment, MAX_COMMENT_SIZE);
    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_NAME, file->var.name, file->var.namelen);
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
    uint8_t *data;
    size_t capacity = 0;
    size_t size = 0;
    int ret;

    ret = convert_alloc_input_buffer(input, &data, &capacity);
    if (ret != 0)
    {
        return ret;
    }

    ret = convert_build_data(input, data, &size,
                             capacity, file, file->compression);
    if (ret < 0)
    {
        free(data);
        return ret;
    }

    ret = convert_build_8x(data, size, file);
    free(data);

    return ret;
}

static int convert_split_output_name(const char *base_name,
                                     unsigned int index,
                                     bool append_suffix,
                                     char *out_name,
                                     size_t out_name_size)
{
    char index_str[16];
    size_t index_len;
    size_t required_len;

    if (base_name == NULL || out_name == NULL || out_name_size == 0)
    {
        return -1;
    }

    sprintf(index_str, "%u", index);
    index_len = strlen(index_str);

    if (append_suffix)
    {
        size_t base_len = strlen(base_name);
        required_len = base_len + 1 + index_len + 4 + 1;
        if (required_len > out_name_size)
        {
            LOG_ERROR("Output path too long for split appvar file.\n");
            return -1;
        }

        memcpy(out_name, base_name, base_len);
        out_name[base_len] = '\0';
        strcat(out_name, ".");
        strcat(out_name, index_str);
        strcat(out_name, ".8xv");
    }
    else
    {
        const char *ext_pos = strrchr(base_name, '.');
        size_t base_len = strlen(base_name);

        if (ext_pos != NULL && strcmp(ext_pos, ".8xv") == 0)
        {
            base_len = (size_t)(ext_pos - base_name);
        }

        required_len = base_len + 1 + index_len + 4 + 1;
        if (required_len > out_name_size)
        {
            LOG_ERROR("Output path too long for split appvar file.\n");
            return -1;
        }

        memcpy(out_name, base_name, base_len);
        out_name[base_len] = '\0';
        strcat(out_name, ".");
        strcat(out_name, index_str);
        strcat(out_name, ".8xv");
    }

    return 0;
}

static int convert_split_append_written_path(char ***paths,
                                             unsigned int *count,
                                             const char *path)
{
    char **new_paths;
    char *path_copy;
    size_t len;

    if (paths == NULL || count == NULL || path == NULL)
    {
        return -1;
    }

    new_paths = realloc(*paths, (size_t)(*count + 1) * sizeof(*new_paths));
    if (new_paths == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return -1;
    }
    *paths = new_paths;

    len = strlen(path);
    path_copy = malloc(len + 1);
    if (path_copy == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    memcpy(path_copy, path, len + 1);
    (*paths)[*count] = path_copy;
    (*count)++;

    return 0;
}

static void convert_split_cleanup_written_paths(char **paths,
                                                unsigned int count,
                                                bool remove_files)
{
    unsigned int i;

    if (paths == NULL)
    {
        return;
    }

    for (i = 0; i < count; ++i)
    {
        if (paths[i] != NULL)
        {
            if (remove_files)
            {
                remove(paths[i]);
            }
            free(paths[i]);
        }
    }

    free(paths);
}

static int convert_split_var_name(const char *base_name,
                                  unsigned int index,
                                  char out_name[TI8X_VAR_NAME_LEN + 1],
                                  size_t *out_len)
{
    char index_str[16];
    size_t index_len;
    size_t base_len;

    if (base_name == NULL || out_name == NULL || out_len == NULL)
    {
        return -1;
    }

    sprintf(index_str, "%u", index);
    index_len = strlen(index_str);
    if (index_len >= TI8X_VAR_NAME_LEN + 1)
    {
        LOG_ERROR("AppVar index too large.\n");
        return -1;
    }

    base_len = strlen(base_name);
    if (base_len > TI8X_VAR_NAME_LEN - index_len)
    {
        base_len = TI8X_VAR_NAME_LEN - index_len;
    }

    memset(out_name, 0, TI8X_VAR_NAME_LEN + 1);
    memcpy(out_name, base_name, base_len);
    memcpy(out_name + base_len, index_str, index_len);

    *out_len = base_len + index_len;

    return 0;
}

static int convert_write_split_appvars(const uint8_t *data,
                                       size_t data_size,
                                       size_t appvar_size,
                                       const struct output_file *file,
                                       unsigned int max_appvars,
                                       bool append_output_suffix,
                                       char (*appvar_names)[10],
                                       unsigned int *out_num_appvars)
{
    struct output_file appvarfile;
    char **written_paths = NULL;
    unsigned int num_appvars;
    unsigned int written_count = 0;
    unsigned int i;
    int ret = -1;

    if (data == NULL || file == NULL || appvar_size == 0)
    {
        LOG_ERROR("Invalid split appvar params.\n");
        return -1;
    }

    if (data_size == 0)
    {
        LOG_ERROR("Input too small for split appvars.\n");
        return -1;
    }

    num_appvars = (unsigned int)((data_size + appvar_size - 1) / appvar_size);
    if (num_appvars > max_appvars)
    {
        if (max_appvars == 99)
        {
            LOG_ERROR("Data too large, would require more than 99 AppVars.\n");
        }
        else
        {
            LOG_ERROR("Input too large.\n");
        }
        return -1;
    }

    if (num_appvars == 1)
    {
        LOG_WARNING("Input too large; split across 1 appvar...\n");
    }
    else
    {
        LOG_WARNING("Input too large; split across %u appvars...\n",
            num_appvars);
    }

    if (appvar_names != NULL)
    {
        memset(appvar_names, 0, num_appvars * sizeof *appvar_names);
    }

    memset(&appvarfile, 0, sizeof appvarfile);

    for (i = 0; i < num_appvars; ++i)
    {
        size_t offset = (size_t)i * appvar_size;
        size_t chunk_size = appvar_size;
        char outname[4096];
        char var_name[TI8X_VAR_NAME_LEN + 1];
        size_t var_name_len;

        if (offset + chunk_size > data_size)
        {
            chunk_size = data_size - offset;
        }

        ret = convert_split_output_name(file->name, i,
            append_output_suffix, outname, sizeof outname);
        if (ret != 0)
        {
            goto fail;
        }

        ret = convert_split_var_name(file->var.name, i, var_name, &var_name_len);
        if (ret != 0)
        {
            goto fail;
        }

        appvarfile.name = outname;
        appvarfile.format = OFORMAT_8XV;
        appvarfile.var.maxsize = appvar_size;
        memset(appvarfile.var.name, 0, sizeof appvarfile.var.name);
        memcpy(appvarfile.var.name, var_name, var_name_len);
        appvarfile.var.namelen = var_name_len;
        appvarfile.var.type = TI8X_TYPE_APPVAR;
        appvarfile.var.archive = true;
        appvarfile.append = false;

        ret = convert_build_8x((uint8_t *)data + offset, chunk_size, &appvarfile);
        if (ret != 0)
        {
            goto fail;
        }

        ret = output_write_file(&appvarfile);
        if (ret != 0)
        {
            goto fail;
        }

        ret = convert_split_append_written_path(&written_paths,
            &written_count, outname);
        if (ret != 0)
        {
            goto fail;
        }

        LOG_PRINT("[success] %s (%s), %lu bytes.\n",
            appvarfile.name,
            var_name,
            (unsigned long)appvarfile.size);

        if (appvar_names != NULL)
        {
            appvar_names[i][0] = TI8X_TYPE_APPVAR;
            memcpy(&appvar_names[i][1], var_name, var_name_len);
        }
    }

    if (appvarfile.data != NULL)
    {
        free(appvarfile.data);
        appvarfile.data = NULL;
        appvarfile.data_capacity = 0;
    }
    convert_split_cleanup_written_paths(written_paths, written_count, false);

    if (out_num_appvars != NULL)
    {
        *out_num_appvars = num_appvars;
    }

    return 0;

fail:
    if (appvarfile.data != NULL)
    {
        free(appvarfile.data);
        appvarfile.data = NULL;
        appvarfile.data_capacity = 0;
    }
    convert_split_cleanup_written_paths(written_paths, written_count, true);
    return ret;
}

static int convert_8xp(struct input *input, struct output_file *file)
{
    uint8_t *data;
    size_t capacity = 0;
    size_t size = 0;
    int ret;

    if (file->compression != COMPRESS_NONE)
    {
        LOG_WARNING("Ignoring compression mode!\n");
    }

    ret = convert_alloc_input_buffer(input, &data, &capacity);
    if (ret != 0)
    {
        return ret;
    }

    ret = convert_build_data(input, data, &size,
        capacity, file, COMPRESS_NONE);
    if (ret < 0)
    {
        free(data);
        return ret;
    }

    if (file->format == OFORMAT_8XP_COMPRESSED)
    {
        file->uncompressed_size = size;
        ret = compress_8xp(data, &size, file->ti8xp_compression);
        if (ret < 0)
        {
            free(data);
            return ret;
        }

        file->compressed = ret == 0;
        file->compressed_size = size;
    }

    if (size > file->var.maxsize)
    {
        unsigned int num_appvars;
        char (*appvar_names)[10];
        size_t split_offset = TI8X_ASMCOMP_LEN;
        size_t payload_size;

        if (size <= split_offset)
        {
            LOG_ERROR("Input too small for split 8xp output.\n");
            free(data);
            return -1;
        }
        payload_size = size - split_offset;

        num_appvars = (unsigned int)((payload_size + file->var.maxsize - 1) /
            file->var.maxsize);
        if (num_appvars > 99)
        {
            LOG_ERROR("Input too large.\n");
            free(data);
            return -1;
        }

        appvar_names = calloc(num_appvars, sizeof *appvar_names);
        if (appvar_names == NULL)
        {
            LOG_ERROR("Out of memory.\n");
            free(data);
            return -1;
        }

        if (data[0] != TI8X_TOKEN_EXT || data[1] != TI8X_TOKEN_ASM84CECMP)
        {
            LOG_WARNING("Input does not start with ASM tokens; split output will replace first two bytes.\n");
        }

        ret = convert_write_split_appvars(
            data + split_offset,
            payload_size,
            file->var.maxsize,
            file,
            99,
            true,
            appvar_names,
            &num_appvars);
        if (ret != 0)
        {
            free(appvar_names);
            free(data);
            return ret;
        }

        /* write extractor program as final output */
        ret = extract_8xp_to_8xv(data, &size, appvar_names, num_appvars);
        free(appvar_names);
        if (ret == 0)
        {
            ret = convert_build_8x(data, size, file);
        }
    }
    else
    {
        ret = convert_build_8x(data, size, file);
    }

    free(data);
    return ret;
}

int convert_auto_8xg(struct input *input, struct output_file *file)
{
    uint8_t *data;
    uint8_t *ti8x;
    uint16_t checksum;
    size_t file_size;
    size_t data_size;
    size_t size;
    int ret;

    if (file->compression != COMPRESS_NONE)
    {
        LOG_WARNING("Ignoring compression mode!\n");
    }

    ret = convert_alloc_input_buffer(input, &data, NULL);
    if (ret != 0)
    {
        return ret;
    }

    ret = convert_build_data(input, data, &size,
                             file->var.maxsize, file, COMPRESS_NONE);
    if (ret < 0)
    {
        free(data);
        return ret;
    }

    file_size = size + TI8X_FILE_HEADER_LEN + TI8X_CHECKSUM_LEN;
    data_size = size;

    if (output_reserve_data(file, file_size) != 0)
    {
        return -1;
    }

    ti8x = file->data;
    file->size = file_size;
    memset(ti8x, 0, file_size);

    memcpy(ti8x + TI8X_FILE_HEADER, ti8x_file_header, sizeof ti8x_file_header);
    memcpy(ti8x + TI8X_VAR_HEADER, data, size);

    ti8x[TI8X_DATA_SIZE + 0] = (data_size >> 0) & 0xff;
    ti8x[TI8X_DATA_SIZE + 1] = (data_size >> 8) & 0xff;

    checksum = ti8x_checksum(ti8x, data_size);

    ti8x[file_size - 2] = (checksum >> 0) & 0xff;
    ti8x[file_size - 1] = (checksum >> 8) & 0xff;

    free(data);
    return 0;
}

static int convert_8ek(struct input *input, struct output_file *file)
{
    uint8_t *input_data;
    const uint8_t *reloc_table;
    uint8_t *output_data;
    uint8_t *ptr;
    uint8_t *app_header_start;
    size_t input_size = 0;
    size_t app_total_size = 0;
    size_t app_data_size;
    size_t reloc_size = 0;
    size_t i;
    uint32_t tmp;
    size_t output_size;

    if (file->compression != COMPRESS_NONE)
    {
        LOG_WARNING("Ignoring compression mode!\n");
    }

    if (input->nr_files != 1)
    {
        LOG_ERROR("8ek format requires exactly one input file.\n");
        return -1;
    }

    reloc_size = input->files[0].reloc_table.size;
    reloc_table = input->files[0].reloc_table.data;
    input_data = input->files[0].data;
    input_size = input->files[0].size;

    app_total_size = input_size + reloc_size + file->description_size + (file->description_size ? 1 : 0);
    app_data_size =
        TI8EK_APP_HEADER_SIZE +
        TI8EK_APP_METADATA_SIZE +
        app_total_size +
        TI8EK_APP_SIGNATURE_FIELD_SIZE +
        TI8EK_APP_SIGNATURE_TYPE_SIZE +
        TI8EK_APP_SIGNATURE_SIZE;
    output_size = TI8X_FILE_HEADER_LEN + TI8X_VAR_HEADER_LEN + app_data_size;

    if (output_reserve_data(file, output_size) != 0)
    {
        return -1;
    }

    output_data = file->data;
    memset(output_data, 0, output_size);

    /* File Header */
    ptr = &output_data[TI8X_FILE_HEADER];
    *ptr++ = '*';
    *ptr++ = '*';
    *ptr++ = 'T';
    *ptr++ = 'I';
    *ptr++ = 'F';
    *ptr++ = 'L';
    *ptr++ = '*';
    *ptr++ = '*';
    *ptr++ = 0x05;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = file->var.namelen;
    for (i = 0; i < file->var.namelen; ++i)
    {
        *ptr++ = file->var.name[i];
    }
    ptr = &output_data[0x30];
    *ptr++ = 0x73;
    *ptr++ = 0x24;
    ptr = &output_data[0x49];
    *ptr++ = 0x13;
    ptr = &output_data[TI8X_DATA];
    *ptr++ = app_data_size >> 0;
    *ptr++ = app_data_size >> 8;
    *ptr++ = app_data_size >> 16;
    *ptr++ = app_data_size >> 24;

    /* Application Header */
    ptr = app_header_start = &output_data[TI8EK_APP_HEADER_OFFSET];
    *ptr++ = 0x81;
    *ptr++ = 0x0F;
    *ptr++ = app_total_size >> 24;
    *ptr++ = app_total_size >> 16;
    *ptr++ = app_total_size >> 8;
    *ptr++ = app_total_size >> 0;
    *ptr++ = 0x81;
    *ptr++ = 0x12;
    *ptr++ = 0x13;
    *ptr++ = 0x0F;
    *ptr++ = 0x81;
    *ptr++ = 0x2D;
    *ptr++ = 0x0B;
    *ptr++ = '5';
    *ptr++ = '.';
    *ptr++ = '0';
    *ptr++ = '.';
    *ptr++ = '0';
    *ptr++ = '.';
    *ptr++ = '0';
    *ptr++ = '0';
    *ptr++ = '8';
    *ptr++ = '9';
    *ptr++ = 0x00;
    *ptr++ = 0x81;
    *ptr++ = 0x32;
    *ptr++ = 0x59;
    *ptr++ = 0x00;
    *ptr++ = 0x81;
    *ptr++ = 0x40 | file->var.namelen;
    for (i = 0; i < file->var.namelen; ++i)
    {
        *ptr++ = file->var.name[i];
    }
    *ptr++ = 0x81;
    *ptr++ = 0xA1;
    *ptr++ = 0x07;
    *ptr++ = 0x03;
    *ptr++ = 0x26;
    *ptr++ = 0x09;
    *ptr++ = 0x04;
    *ptr++ = 0x21;
    *ptr++ = 0xBB;
    *ptr++ = 0x6E;
    *ptr++ = 0xDC;
    *ptr++ = 0x00;
    *ptr++ = 0x0D;
    *ptr = TI8EK_APP_HEADER_SIZE - (ptr - app_header_start);
    ptr = &output_data[TI8EK_APP_HEADER_SIZE_FIELD_OFFSET];
    *ptr++ = 0x81;
    *ptr++ = 0x7F;
    *ptr++ = app_total_size >> 24;
    *ptr++ = app_total_size >> 16;
    *ptr++ = app_total_size >> 8;
    *ptr++ = app_total_size >> 0;

    /* Application Metadata */
    ptr = &output_data[TI8EK_APP_METADATA_OFFSET];
    *ptr++ = 'e';
    *ptr++ = 'Z';
    *ptr++ = '8';
    ptr = &output_data[TI8EK_APP_METADATA_NAME_OFFSET];
    for (i = 0; i < file->var.namelen; ++i)
    {
        *ptr++ = file->var.name[i];
    }
    ptr = &output_data[TI8EK_APP_METADATA_FLAGS_OFFSET];
    *ptr++ = (1 << 0);
    ptr = &output_data[TI8EK_APP_METADATA_OFFSET + 0x12];
    tmp = 0x2A + reloc_size;
    *ptr++ = tmp >> 0;
    *ptr++ = tmp >> 8;
    *ptr++ = tmp >> 16;
    ptr = &output_data[TI8EK_APP_METADATA_OFFSET + 0x1B];
    tmp = 0x2A + reloc_size;
    *ptr++ = tmp >> 0;
    *ptr++ = tmp >> 8;
    *ptr++ = tmp >> 16;
    ptr = &output_data[TI8EK_APP_METADATA_OFFSET + 0x24];
    tmp = 0x2A + reloc_size + input_size;
    *ptr++ = tmp >> 0;
    *ptr++ = tmp >> 8;
    *ptr++ = tmp >> 16;
    ptr = &output_data[TI8EK_APP_METADATA_RELOC_OFFSET];

    if (reloc_size > 0)
    {
        memcpy(ptr, reloc_table, reloc_size);
        ptr += reloc_size;
    }

    memcpy(ptr, input_data, input_size);
    ptr += input_size;

    if (file->description_size > 0)
    {
        memcpy(ptr, file->description, file->description_size);
        ptr += file->description_size;
        *ptr++ = 0x00;
    }

    *ptr++ = 0x02;
    *ptr++ = 0x3E;
    *ptr++ = 0x01;
    *ptr++ = 0x00;
    for (i = 0; i < TI8EK_APP_SIGNATURE_SIZE; ++i)
    {
        *ptr++ = 0xFF;
    }

    file->size = output_size;

    return 0;
}

static int convert_8xv_split(struct input *input, struct output_file *file)
{
    uint8_t *data;
    size_t capacity = 0;
    size_t size = 0;
    int ret;

    if (file->compression != COMPRESS_NONE)
    {
        LOG_WARNING("Ignoring compression mode!\n");
    }

    ret = convert_alloc_input_buffer(input, &data, &capacity);
    if (ret != 0)
    {
        return ret;
    }

    ret = convert_build_data(input, data, &size,
                             capacity, file, COMPRESS_NONE);
    if (ret < 0)
    {
        free(data);
        return ret;
    }

    ret = convert_write_split_appvars(
        data,
        size,
        file->var.maxsize,
        file,
        99,
        false,
        NULL,
        NULL);
    free(data);
    return ret;
}

int convert_bin(struct input *input, struct output_file *file)
{
    size_t max_size = 0;
    uint32_t i;

    for (i = 0; i < input->nr_files; ++i)
    {
        if (max_size > (size_t)-1 - input->files[i].size)
        {
            LOG_ERROR("Input too large.\n");
            return -1;
        }
        max_size += input->files[i].size;
    }

    if (max_size == 0 && file->compression == COMPRESS_NONE)
    {
        if (output_reserve_data(file, 1) != 0)
        {
            return -1;
        }
        file->size = 0;
        return 0;
    }

    if (output_reserve_data(file, max_size == 0 ? 1 : max_size) != 0)
    {
        return -1;
    }

    return convert_build_data(input,
        file->data,
        &file->size,
        max_size,
        file,
        file->compression);
}

int convert_normal(struct input *input, struct output *output)
{
    struct output_file *file = &output->file;
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

    switch (file->format)
    {
        case OFORMAT_C:
        case OFORMAT_ASM:
        case OFORMAT_ICE:
        case OFORMAT_BIN:
            ret = convert_bin(input, file);
            break;

        case OFORMAT_8XV:
        case OFORMAT_8XG:
            ret = convert_8x(input, file);
            break;

        case OFORMAT_8XP:
        case OFORMAT_8XP_COMPRESSED:
            ret = convert_8xp(input, file);
            break;

        case OFORMAT_8XG_AUTO_EXTRACT:
            ret = convert_auto_8xg(input, file);
            break;

        case OFORMAT_8EK:
            ret = convert_8ek(input, file);
            break;

        case OFORMAT_8XV_SPLIT:
            return convert_8xv_split(input, file);

        default:
            ret = -1;
            break;
    }

    if (ret != 0)
    {
        return ret;
    }

    ret = output_write_file(file);
    if (ret != 0)
    {
        return ret;
    }

    if ((file->compression || file->format == OFORMAT_8XP_COMPRESSED) &&
        file->compressed)
    {
        float savings = (float)file->uncompressed_size -
                        (float)file->compressed_size;

        if (savings < 0.001)
        {
            savings = 0;
        }
        else
        {
            if (file->uncompressed_size != 0)
            {
                savings /= (float)file->uncompressed_size;
            }
        }

        const char *compress_str =
            (file->format == OFORMAT_8XP_COMPRESSED && file->ti8xp_compression == COMPRESS_ZX7) ||
            (file->compression == COMPRESS_ZX7) ? "zx7" : "zx0";

        LOG_PRINT("[success] %s, %lu bytes. (%s compressed %.2f%%)\n",
            file->name,
            (unsigned long)file->size,
            compress_str,
            savings * 100.0);
    }
    else
    {
        if (file->format == OFORMAT_8EK)
        {
            LOG_PRINT("[success] %s, %lu bytes. (%lu relocations)\n",
                file->name,
                (unsigned long)file->size,
                (unsigned long)(input->files[0].reloc_table.size / 6));
        }
        else
        {
            LOG_PRINT("[success] %s, %lu bytes.\n",
                file->name,
                (unsigned long)file->size);
        }
    }

    return 0;
}

int convert_zip(struct input *input, struct output *output)
{
    const char *archive_path = output->file.name;
    static mz_zip_archive archive;
    static mz_zip_archive_file_stat stat;
    size_t i;
    uint32_t checksum = 0;
    int writer_initialized = 0;
    int ret = -1;

    if (!archive_path || strlen(archive_path) < 1)
    {
        LOG_ERROR("Invalid archive filepath.\n");
        return -1;
    }

    if (!mz_zip_writer_init_file(&archive, archive_path, 0))
    {
        LOG_ERROR("Could not initialize archive filepath.\n");
        return -1;
    }
    writer_initialized = 1;

    if (output->file.format == OFORMAT_B83 ||
        output->file.format == OFORMAT_B84)
    {
        static const char b83metadata[] =
            "bundle_identifier:TI Bundle\r\n"
            "bundle_format_version:1\r\n"
            "bundle_target_device:83CE\r\n"
            "bundle_target_type:CUSTOM\r\n"
            "bundle_comments:Created by convbin " VERSION_STRING "\r\n";
        static const char b84metadata[] =
            "bundle_identifier:TI Bundle\r\n"
            "bundle_format_version:1\r\n"
            "bundle_target_device:84CE\r\n"
            "bundle_target_type:CUSTOM\r\n"
            "bundle_comments:Created by convbin " VERSION_STRING "\r\n";

        if (!mz_zip_writer_add_mem(
            &archive,
            "METADATA",
            output->file.format == OFORMAT_B83 ? b83metadata : b84metadata,
            (output->file.format == OFORMAT_B83 ? sizeof b83metadata : sizeof b84metadata) - 1,
            MZ_BEST_COMPRESSION))
        {
            LOG_ERROR("Could not add bundle metadata.\n");
            goto cleanup;
        }

        if (!mz_zip_reader_file_stat(&archive, 0, &stat))
        {
            LOG_ERROR("Could not stat bundle metadata.\n");
            goto cleanup;
        }

        LOG_DEBUG("metadata checksum: %x\n", stat.m_crc32);
        checksum += stat.m_crc32;
    }

    for (i = 0; i < input->nr_files; ++i)
    {
        const char *filepath = input->files[i].name;
        const char *basename;

        basename = strrchr(filepath, '/');
        if (!basename)
        {
            basename = strrchr(filepath, '\\');
        }
        if (!basename)
        {
            basename = filepath;
        }
        else
        {
            basename += 1;
        }

        if (!mz_zip_writer_add_file(
            &archive,
            basename,
            filepath,
            "",
            0,
            MZ_BEST_COMPRESSION))
        {
            LOG_ERROR("Could not add archive entry.\n");
            goto cleanup;
        }

        /* compute checksum only if b83 or b84 */
        if (output->file.format != OFORMAT_ZIP)
        {
            if (!mz_zip_reader_file_stat(&archive, i + 1, &stat))
            {
                LOG_ERROR("Could not stat archive entry.\n");
                goto cleanup;
            }

            LOG_DEBUG("entry checksum: %x\n", stat.m_crc32);
            checksum += stat.m_crc32;
        }
    }

    if (output->file.format == OFORMAT_B83 ||
        output->file.format == OFORMAT_B84)
    {
        char checksum_str[11];
        const size_t checksum_size =
            sprintf(checksum_str, "%x\r\n", checksum);

        LOG_DEBUG("checksum: %x\n", checksum);

        if (!mz_zip_writer_add_mem(
            &archive,
            "_CHECKSUM",
            checksum_str,
            checksum_size,
            MZ_BEST_COMPRESSION))
        {
            LOG_ERROR("Could not add bundle checksum.\n");
            goto cleanup;
        }
    }

    if (!mz_zip_writer_finalize_archive(&archive))
    {
        LOG_ERROR("Could not finalize archive.\n");
        goto cleanup;
    }

    LOG_PRINT("[success] %lu files added to %s.\n",
        (unsigned long)input->nr_files, archive_path);

    ret = 0;

cleanup:
    if (writer_initialized)
    {
        mz_zip_writer_end(&archive);
    }

    return ret;
}

int convert_input_to_output(struct input *input, struct output *output)
{
    /* if a bundle, zip input files directly */
    if (output->file.format == OFORMAT_B84 ||
        output->file.format == OFORMAT_B83 ||
        output->file.format == OFORMAT_ZIP)
    {
        return convert_zip(input, output);
    }
    else
    {
        return convert_normal(input, output);
    }
}
