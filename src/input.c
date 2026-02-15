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

#include "input.h"
#include "ti8x.h"
#include "elf.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <ctype.h>

static int input_read_range(FILE *fd,
                            size_t offset,
                            size_t strip_end_bytes,
                            uint8_t **data,
                            size_t *size)
{
    long file_size_long;
    size_t file_size;
    size_t read_size;
    uint8_t *buffer;

    if (fd == NULL || data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in '%s'.\n", __func__);
        return -1;
    }

    if (fseek(fd, 0, SEEK_END) != 0)
    {
        LOG_ERROR("Input seek failed.\n");
        return -1;
    }

    file_size_long = ftell(fd);
    if (file_size_long < 0)
    {
        LOG_ERROR("Input size failed.\n");
        return -1;
    }

    file_size = (size_t)file_size_long;

    if (file_size < offset + strip_end_bytes)
    {
        LOG_ERROR("Input file too short.\n");
        return -1;
    }

    read_size = file_size - offset - strip_end_bytes;

    buffer = malloc(read_size == 0 ? 1 : read_size);
    if (buffer == NULL)
    {
        LOG_ERROR("Memory error in '%s'.\n", __func__);
        return -1;
    }

    if (fseek(fd, offset, SEEK_SET) != 0)
    {
        LOG_ERROR("Input seek failed.\n");
        free(buffer);
        return -1;
    }

    if (read_size > 0 && fread(buffer, 1, read_size, fd) != read_size)
    {
        LOG_ERROR("Input read failed.\n");
        free(buffer);
        return -1;
    }

    *data = buffer;
    *size = read_size;

    return 0;
}

static int input_bin(FILE *fd, uint8_t **data, size_t *size, size_t offset)
{
    return input_read_range(fd, offset, 0, data, size);
}

static int input_ti8x(FILE *fd, uint8_t **data, size_t *size, size_t offset)
{
    return input_read_range(fd, offset, TI8X_CHECKSUM_LEN, data, size);
}

static char *input_csv_line(FILE *fd)
{
    char *line = NULL;
    int i = 0;
    int c;

    do
    {
        c = fgetc(fd);

        if (c < ' ')
        {
            c = 0;
        }

        if (!isspace((unsigned char)c))
        {
#define REALLOC_BYTES 512
            if (i % REALLOC_BYTES == 0)
            {
                void *tmp = line;
                line = realloc(tmp, ((i / REALLOC_BYTES) + 1) * REALLOC_BYTES);
                if (line == NULL)
                {
                    LOG_ERROR("Memory error in '%s'.\n", __func__);
                    free(tmp);
                    return NULL;
                }
            }
#undef REALLOC_BYTES
            line[i] = (char)c;
            i++;
        }
    } while (c);

    return line;
}

static int input_csv(FILE *fd, uint8_t **data, size_t *size)
{
    size_t s = 0;
    size_t cap = 512;
    uint8_t *buffer;

    if (data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in '%s'.\n", __func__);
        return -1;
    }

    buffer = malloc(cap);
    if (buffer == NULL)
    {
        LOG_ERROR("Memory error in '%s'.\n", __func__);
        return -1;
    }

    do
    {
        char *token;
        char *line;

        line = input_csv_line(fd);
        if (line == NULL)
        {
            free(buffer);
            return -1;
        }

        token = strtok(line, ",");

        while (token)
        {
            long int value = strtol(token, NULL, 0);

            if (s == cap)
            {
                size_t next_cap;
                uint8_t *tmp;

                if (cap > SIZE_MAX / 2)
                {
                    LOG_ERROR("CSV input too large.\n");
                    free(line);
                    free(buffer);
                    return -1;
                }
                next_cap = cap * 2;

                tmp = realloc(buffer, next_cap);
                if (tmp == NULL)
                {
                    LOG_ERROR("Memory error in '%s'.\n", __func__);
                    free(line);
                    free(buffer);
                    return -1;
                }

                buffer = tmp;
                cap = next_cap;
            }

            buffer[s++] = (uint8_t)(value < 0 || value > 255 ? 255 : value);

            token = strtok(NULL, ",");
        }

        free(line);
    } while (!feof(fd));

    *data = buffer;
    *size = s;

    return 0;
}

static int input_elf(FILE *fd,
                     uint8_t **data,
                     size_t *size,
                     struct app_reloc_table *reloc_table)
{
    return elf_extract_binary(fd, data, size, reloc_table);
}

int input_read_file(struct input_file *file)
{
    FILE *fd;
    int ret;

    if (file->data != NULL)
    {
        free(file->data);
        file->data = NULL;
        file->size = 0;
    }

    if (file->reloc_table.data != NULL)
    {
        free(file->reloc_table.data);
        file->reloc_table.data = NULL;
        file->reloc_table.size = 0;
    }

    fd = fopen(file->name, "rb");
    if (fd == NULL)
    {
        LOG_ERROR("Cannot open input file '%s': %s\n",
            file->name,
            strerror(errno));
        return -1;
    }

    switch (file->format)
    {
        case IFORMAT_BIN:
            ret = input_bin(fd, &file->data, &file->size, 0);
            break;

        case IFORMAT_TI8X_DATA:
            ret = input_ti8x(fd, &file->data, &file->size, TI8X_DATA);
            break;

        case IFORMAT_TI8X_DATA_VAR:
            ret = input_ti8x(fd, &file->data, &file->size, TI8X_VAR_HEADER);
            break;

        case IFORMAT_TI8EK:
            ret = input_bin(fd, &file->data, &file->size, TI8EK_APP_HEADER_OFFSET);
            break;

        case IFORMAT_CSV:
            ret = input_csv(fd, &file->data, &file->size);
            break;

        case IFORMAT_ELF:
            ret = input_elf(fd, &file->data, &file->size, &file->reloc_table);
            break;

        default:
            LOG_ERROR("Unknown input format.\n");
            ret = -1;
            break;
    }

    fclose(fd);

    if (ret != 0)
    {
        if (file->data != NULL)
        {
            free(file->data);
            file->data = NULL;
            file->size = 0;
        }
    }

    return ret;
}

int input_add_file_path(struct input *input, const char *path)
{
    struct input_file *f;

    if (input->nr_files >= INPUT_MAX_NUM)
    {
        LOG_ERROR("Too many input files.\n");
        return -1;
    }

    f = &input->files[input->nr_files];

    f->name = path;
    f->format = input->default_format;
    f->compression = input->default_compression;
    f->size = 0;
    f->data = NULL;
    f->reloc_table.data = NULL;
    f->reloc_table.size = 0;

    input->nr_files++;

    return 0;
}

void input_free_files(struct input *input)
{
    uint32_t i;

    if (input == NULL)
    {
        return;
    }

    for (i = 0; i < input->nr_files; ++i)
    {
        if (input->files[i].data != NULL)
        {
            free(input->files[i].data);
            input->files[i].data = NULL;
            input->files[i].size = 0;
        }

        if (input->files[i].reloc_table.data != NULL)
        {
            free(input->files[i].reloc_table.data);
            input->files[i].reloc_table.data = NULL;
            input->files[i].reloc_table.size = 0;
        }
    }
}
