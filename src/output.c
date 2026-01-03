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

#include "output.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define VALUES_PER_LINE 32

static int output_c(const char *name, const unsigned char *data, size_t size, FILE *fd)
{
    size_t i;

    fprintf(fd, "unsigned char %s[%lu] =\n{", name, (unsigned long)size);
    for (i = 0; i < size; ++i)
    {
        bool last = i + 1 == size;

        if (i % VALUES_PER_LINE == 0)
        {
            fputs("\n    ", fd);
        }

        if (last)
        {
            fprintf(fd, "0x%02x", data[i]);
        }
        else
        {
            fprintf(fd, "0x%02x,", data[i]);
        }
    }
    fputs("\n};\n", fd);

    return 0;
}

static int output_asm(const char *name, const unsigned char *data, size_t size, FILE *fd)
{
    size_t i;

    fprintf(fd, "%s:\n", name);
    fprintf(fd, "; %lu bytes\n\tdb\t", (unsigned long)size);
    for (i = 0; i < size; ++i)
    {
        bool last = i + 1 == size;

        if (last || ((i + 1) % VALUES_PER_LINE == 0))
        {
            fprintf(fd, "$%02x", data[i]);
            if (!last)
            {
                fputs("\n\tdb\t", fd);
            }
        }
        else
        {
             fprintf(fd, "$%02x,", data[i]);
        }
    }
    fputc('\n', fd);

    return 0;
}

static int output_ice(const char *name, const unsigned char *data, size_t size, FILE *fd)
{
    size_t i;

    fprintf(fd, "%s | %lu bytes\n\"", name, (unsigned long)size);

    for (i = 0; i < size; ++i)
    {
        fprintf(fd, "%02X", data[i]);
    }

    fprintf(fd, "\"\n");

    return 0;
}

static int output_bin(const char *name, const unsigned char *data, size_t size, FILE *fd)
{
    int ret = fwrite(data, size, 1, fd);
    (void)name;

    return ret == 1 ? 0 : -1;
}

int output_write_file(const struct output_file *file)
{
    FILE *fd;
    int ret;

    fd = fopen(file->name, file->append ? "ab" : "wb");
    if (fd == NULL)
    {
        LOG_ERROR("Cannot open output file \'%s\': %s\n",
            file->name,
            strerror(errno));
        return -1;
    }

    switch (file->format)
    {
        case OFORMAT_C:
            ret = output_c(file->var.name, file->data, file->size, fd);
            break;

        case OFORMAT_ASM:
            ret = output_asm(file->var.name, file->data, file->size, fd);
            break;

        case OFORMAT_ICE:
            ret = output_ice(file->var.name, file->data, file->size, fd);
            break;

        case OFORMAT_BIN:
        case OFORMAT_8XV:
        case OFORMAT_8XP:
        case OFORMAT_8XG:
        case OFORMAT_8XG_AUTO_EXTRACT:
        case OFORMAT_8XP_COMPRESSED:
            ret = output_bin(file->var.name, file->data, file->size, fd);
            break;

        default:
            LOG_ERROR("Unknown output format.\n");
            ret = -1;
            break;
    }

    fclose(fd);

    return ret;
}

void output_set_varname(struct output *output, const char *varname)
{
    if (varname == NULL)
    {
        return;
    }

    size_t len = strlen(varname);
    size_t i;

    if (len > TI8X_VAR_MAX_NAME_LEN)
    {
        len = TI8X_VAR_MAX_NAME_LEN;
        LOG_WARNING("Variable name truncated to %u characters\n",
            (unsigned int)len);
    }

    for (i = 0; i < len; ++i)
    {
        if (output->file.uppercase && isalpha(varname[i]))
        {
            output->file.var.name[i] = toupper(varname[i]);
        }
        else
        {
            output->file.var.name[i] = varname[i];
        }
    }

    output->file.var.name[len] = '\0';
}
