/*
 * Copyright 2017-2022 Matt "MateoConLechuga" Waltz
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
#include "log.h"

#include <errno.h>
#include <string.h>
#include <ctype.h>

static int input_bin(FILE *fd, uint8_t *data, size_t *size)
{
    size_t s = 0;

    if (fd == NULL || data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'.\n", __func__);
        return -1;
    }

    for (;;)
    {
        int c = fgetc(fd);
        if (c == EOF)
        {
            break;
        }

        data[s++] = (uint8_t)c;
        if (s >= INPUT_MAX_SIZE)
        {
            LOG_ERROR("Input file too large.\n");
            return -1;
        }
    }

    *size = s;

    return 0;
}

static int input_ti8x(FILE *fd, uint8_t *data, size_t *size, bool header)
{
    size_t s = 0;
    int ret;

    if (fd == NULL || data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'.\n", __func__);
        return -1;
    }

    ret = fseek(fd, header ? TI8X_VAR_HEADER : TI8X_DATA, SEEK_SET);
    if (ret != 0)
    {
        LOG_ERROR("Input seek failed.\n");
        return -1;
    }

    for (;;)
    {
        int c = fgetc(fd);
        if (c == EOF)
        {
            break;
        }

        data[s++] = (uint8_t)c;
        if (s >= INPUT_MAX_SIZE)
        {
            LOG_ERROR("Input file too large.\n");
            return -1;
        }
    }

    if (s >= TI8X_CHECKSUM_LEN)
    {
        s -= TI8X_CHECKSUM_LEN;
    }
    else
    {
        LOG_ERROR("Input file too short.\n");
        return -1;
    }

    *size = s;

    return 0;
}

static int input_ti8x_data(FILE *fd, uint8_t *data, size_t *size)
{
    return input_ti8x(fd, data, size, false);
}

static int input_ti8x_data_var(FILE *fd, uint8_t *data, size_t *size)
{
    return input_ti8x(fd, data, size, true);
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
#define REALLOC_BYTES 384
            if (i % REALLOC_BYTES == 0)
            {
                line = realloc(line, ((i / REALLOC_BYTES) + 1) * REALLOC_BYTES);
                if (line == NULL)
                {
                    LOG_ERROR("Memory error in \'%s\'.\n", __func__);
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
            data[s++] = (uint8_t)(value < 0 || value > 255 ? 255 : value);

            if (s > INPUT_MAX_SIZE)
            {
                LOG_ERROR("Exceeded maximum csv values.\n");
                free(line);
                return -1;
            }

            token = strtok(NULL, ",");
        }

        free(line);
    } while (!feof(fd));

    *size = s;

    return 0;
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
            ret = input_bin(fd, file->data, &file->size);
            break;

        case IFORMAT_TI8X_DATA:
            ret = input_ti8x_data(fd, file->data, &file->size);
            break;

        case IFORMAT_TI8X_DATA_VAR:
            ret = input_ti8x_data_var(fd, file->data, &file->size);
            break;

        case IFORMAT_CSV:
            ret = input_csv(fd, file->data, &file->size);
            break;

        default:
            LOG_ERROR("Unknown input format.\n");
            ret = -1;
            break;
    }

    fclose(fd);

    return ret;
}
