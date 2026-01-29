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

static int input_bin(FILE *fd, uint8_t *data, size_t *size, size_t offset)
{
    size_t bytes_read;

    if (fd == NULL || data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in '%s'.\n", __func__);
        return -1;
    }

    if (fseek(fd, offset, SEEK_SET) != 0)
    {
        LOG_ERROR("Input seek failed.\n");
        return -1;
    }

    bytes_read = fread(data, 1, INPUT_MAX_SIZE, fd);

    if (bytes_read == 0 && ferror(fd))
    {
        LOG_ERROR("Input read failed.\n");
        return -1;
    }

    if (bytes_read == INPUT_MAX_SIZE && fgetc(fd) != EOF)
    {
        LOG_ERROR("Input file too large.\n");
        return -1;
    }

    *size = bytes_read;

    return 0;
}

static int input_ti8x(FILE *fd, uint8_t *data, size_t *size, size_t offset)
{
    size_t bytes_read;

    if (fd == NULL || data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in '%s'.\n", __func__);
        return -1;
    }

    if (fseek(fd, offset, SEEK_SET) != 0)
    {
        LOG_ERROR("Input seek failed.\n");
        return -1;
    }

    bytes_read = fread(data, 1, INPUT_MAX_SIZE, fd);
    
    if (bytes_read == 0 && ferror(fd))
    {
        LOG_ERROR("Input read failed.\n");
        return -1;
    }

    if (bytes_read == INPUT_MAX_SIZE && fgetc(fd) != EOF)
    {
        LOG_ERROR("Input file too large.\n");
        return -1;
    }

    if (bytes_read >= TI8X_CHECKSUM_LEN)
    {
        bytes_read -= TI8X_CHECKSUM_LEN;
    }
    else
    {
        LOG_ERROR("Input file too short.\n");
        return -1;
    }

    *size = bytes_read;

    return 0;
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

        if (!isspace(c))
        {
#define REALLOC_BYTES 512
            if (i % REALLOC_BYTES == 0)
            {
                void *tmp = line;
                line = realloc(tmp, ((i / REALLOC_BYTES) + 1) * REALLOC_BYTES);
                if (line == NULL)
                {
                    LOG_ERROR("Memory error in \'%s\'.\n", __func__);
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

static int input_csv(FILE *fd, uint8_t *data, size_t *size)
{
    size_t s = 0;

    if (data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'.\n", __func__);
        return -1;
    }

    do
    {
        char *token;
        char *line;

        line = input_csv_line(fd);
        if (line == NULL)
        {
            break;
        }

        token = strtok(line, ",");

        while (token)
        {
            long int value = strtol(token, NULL, 0);

            if (s >= INPUT_MAX_SIZE)
            {
                LOG_ERROR("Exceeded maximum csv values.\n");
                free(line);
                return -1;
            }

            data[s++] = (uint8_t)(value < 0 || value > 255 ? 255 : value);

            token = strtok(NULL, ",");
        }

        free(line);
    } while (!feof(fd));

    *size = s;

    return 0;
}

static int input_elf(FILE *fd, uint8_t *data, size_t *size, struct app_reloc_table *reloc_table)
{
    return elf_extract_binary(fd, data, size, reloc_table);
}

int input_read_file(struct input_file *file)
{
    FILE *fd;
    int ret;

    fd = fopen(file->name, "rb");
    if (fd == NULL)
    {
        LOG_ERROR("Cannot open input file \'%s\': %s\n",
            file->name,
            strerror(errno));
        return -1;
    }

    switch (file->format)
    {
        case IFORMAT_BIN:
            ret = input_bin(fd, file->data, &file->size, 0);
            break;

        case IFORMAT_TI8X_DATA:
            ret = input_ti8x(fd, file->data, &file->size, TI8X_DATA);
            break;

        case IFORMAT_TI8X_DATA_VAR:
            ret = input_ti8x(fd, file->data, &file->size, TI8X_VAR_HEADER);
            break;

        case IFORMAT_TI8EK:
            ret = input_bin(fd, file->data, &file->size, TI8EK_APP_HEADER_OFFSET);
            break;

        case IFORMAT_CSV:
            ret = input_csv(fd, file->data, &file->size);
            break;

        case IFORMAT_ELF:
            ret = input_elf(fd, file->data, &file->size, &file->reloc_table);
            break;

        default:
            LOG_ERROR("Unknown input format.\n");
            ret = -1;
            break;
    }

    fclose(fd);

    return ret;
}

int input_add_file_path(struct input *input, const char *path)
{
    if (input->nr_files >= INPUT_MAX_NUM)
    {
        LOG_ERROR("Too many input files.\n");
        return -1;
    }

    struct input_file *f = &input->files[input->nr_files];

    f->name = path;
    f->format = input->default_format;
    f->compression = input->default_compression;
    f->reloc_table.data = NULL;
    f->reloc_table.size = 0;

    input->nr_files++;

    return 0;
}
