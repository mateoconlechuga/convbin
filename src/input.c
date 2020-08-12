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

#include "input.h"
#include "ti8x.h"
#include "log.h"

#include "deps/zx7/zx7.h"

#include <errno.h>
#include <string.h>
#include <ctype.h>

#define REALLOC_BYTES 256

/*
 * Interprets input as raw binary.
 */
static int input_bin(FILE *fdi, unsigned char *arr, size_t *size)
{
    int s = 0;

    if (fdi == NULL || arr == NULL || size == NULL)
    {
        LL_DEBUG("Invalid param in %s.", __func__);
        return 1;
    }

    for (;;)
    {
        int c = fgetc(fdi);
        if (c == EOF)
        {
            break;
        }

        arr[s++] = (unsigned char)c;
        if (s >= INPUT_MAX_SIZE)
        {
            LL_ERROR("Input file too large.");
            return 1;
        }
    }

    *size = s;

    return 0;
}

/*
 * Interprets input as TI 8x* formatted file.
 */
static int input_ti8x(FILE *fdi, unsigned char *arr, size_t *size,
                      bool header)
{
    int s = 0;
    int ret;

    if (fdi == NULL || arr == NULL || size == NULL)
    {
        LL_DEBUG("Invalid param in %s.", __func__);
        return 1;
    }

    ret = fseek(fdi, header ? TI8X_VAR_HEADER : TI8X_DATA, SEEK_SET);
    if (ret != 0)
    {
        LL_ERROR("Input seek failed.");
        return ret;
    }

    for (;;)
    {
        int c = fgetc(fdi);
        if (c == EOF)
        {
            break;
        }

        arr[s++] = (unsigned char)c;
        if (s >= INPUT_MAX_SIZE)
        {
            LL_ERROR("Input file too large.");
            return 1;
        }
    }

    if (s >= TI8X_CHECKSUM_LEN)
    {
        s -= TI8X_CHECKSUM_LEN;
    }
    else
    {
        LL_ERROR("Input file too short.");
        return 1;
    }

    *size = s;

    return 0;
}

/*
 * Interprets input as TI 8x* data section.
 */
static int input_ti8x_data(FILE *fdi, unsigned char *arr, size_t *size)
{
    return input_ti8x(fdi, arr, size, false);
}

/*
 * Interprets input as TI 8x* data and variable header section.
 */
static int input_ti8x_data_var(FILE *fdi, unsigned char *arr, size_t *size)
{
    return input_ti8x(fdi, arr, size, true);
}

/*
 * Read a line from an input file; strips whitespace.
 * Returns NULL if error.
 */
static char *input_csv_line(FILE *fdi)
{
    char *line = NULL;
    int i = 0;
    int c;

    do
    {
        c = fgetc(fdi);

        if (c < ' ')
        {
            c = 0;
        }

        if (!isspace(c))
        {
            if (i % REALLOC_BYTES == 0)
            {
                line = realloc(line, ((i / REALLOC_BYTES) + 1) * REALLOC_BYTES);
                if (line == NULL)
                {
                    LL_ERROR("%s", strerror(errno));
                    return NULL;
                }
            }

            line[i] = (char)c;
            i++;
        }
    } while (c);

    return line;
}

/*
 * Reads each line of a CSV file and stores into an array.
 */
static int input_csv(FILE *fdi, unsigned char *arr, size_t *size)
{
    char *line;
    size_t s = 0;

    if (arr == NULL || size == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        return 1;
    }

    do
    {
        char *token;

        line = input_csv_line(fdi);
        if (line == NULL)
        {
            break;
        }

        token = strtok(line, ",");

        while (token)
        {
            int value = strtol(token, NULL, 0);
            arr[s++] = (unsigned char)(value == -1 ? 0 : value);

            if (s > INPUT_MAX_SIZE)
            {
                LL_ERROR("Exceeded maximum csv values.");
                free(line);
                return 1;
            }

            token = strtok(NULL, ",");
        }

        free(line);
    } while (!feof(fdi));

    *size = s;

    return 0;
}

/*
 * Reads a file depending on the format.
 */
int input_read_file(input_file_t *file)
{
    FILE *fdi;
    int ret;

    fdi = fopen(file->name, "rb");
    if (fdi == NULL)
    {
        LL_ERROR("Cannot open input file \"%s\": %s",
            file->name,
            strerror(errno));
        return 1;
    }

    switch (file->format)
    {
        case IFORMAT_BIN:
            ret = input_bin(fdi, file->arr, &file->size);
            break;

        case IFORMAT_TI8X_DATA:
            ret = input_ti8x_data(fdi, file->arr, &file->size);
            break;

        case IFORMAT_TI8X_DATA_VAR:
            ret = input_ti8x_data_var(fdi, file->arr, &file->size);
            break;

        case IFORMAT_CSV:
            ret = input_csv(fdi, file->arr, &file->size);
            break;

        default:
            ret = 1;
            break;
    }

    fclose(fdi);

    return ret;
}
