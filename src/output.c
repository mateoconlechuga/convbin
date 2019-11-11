#include "output.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define VALUES_PER_LINE 32

/*
 * Outputs to C format.
 */
static int output_c(const char *name, unsigned char *arr, size_t size, FILE *fdo)
{
    size_t i;

    fprintf(fdo, "unsigned char %s[%lu] =\n{", name, size);
    for (i = 0; i < size; ++i)
    {
        bool last = i + 1 == size;

        if (i % VALUES_PER_LINE == 0)
        {
            fputs("\n    ", fdo);
        }

        if (last)
        {
            fprintf(fdo, "0x%02x", arr[i]);
        }
        else
        {
            fprintf(fdo, "0x%02x,", arr[i]);
        }
    }
    fputs("\n};\n", fdo);

    return 0;
}

/*
 * Outputs to Assembly format.
 */
static int output_asm(const char *name, unsigned char *arr, size_t size, FILE *fdo)
{
    size_t i;

    fprintf(fdo, "%s:\n", name);
    fprintf(fdo, "; %lu bytes\n\tdb\t", size);
    for (i = 0; i < size; ++i)
    {
        bool last = i + 1 == size;

        if (last || ((i + 1) % VALUES_PER_LINE == 0))
        {
            fprintf(fdo, "$%02x", arr[i]);
            if (!last)
            {
                fputs("\n\tdb\t", fdo);
            }
        }
        else
        {
             fprintf(fdo, "$%02x,", arr[i]);
        }
    }
    fputc('\n', fdo);

    return 0;
}

/*
 * Converts to ICE format.
 */
static int output_ice(const char *name, unsigned char *arr, size_t size, FILE *fdo)
{
    size_t i;

    fprintf(fdo, "%s | %lu bytes\n\"", name, size);

    for (i = 0; i < size; ++i)
    {
        fprintf(fdo, "%02X", arr[i]);
    }

    fprintf(fdo, "\"\n");

    return 0;
}

/*
 * Outputs to Binary format.
 */
static int output_bin(const char *name, unsigned char *arr, size_t size, FILE *fdo)
{
    int ret = fwrite(arr, size, 1, fdo);
    (void)name;

    return ret == 1 ? 0 : 1;
}

/*
 * Outputs the converted information to a file.
 */
int output_write_file(output_file_t *file)
{
    FILE *fdo;
    int ret;

    fdo = fopen(file->name, file->append ? "ab" : "wb");
    if (fdo == NULL)
    {
        LL_ERROR("cannot open output file: %s", strerror(errno));
        return 1;
    }

    switch (file->format)
    {
        case OFORMAT_C:
            ret = output_c(file->name, file->arr, file->size, fdo);
            break;

        case OFORMAT_ASM:
            ret = output_asm(file->name, file->arr, file->size, fdo);
            break;

        case OFORMAT_ICE:
            ret = output_ice(file->name, file->arr, file->size, fdo);
            break;

        case OFORMAT_BIN:
        case OFORMAT_8XV:
        case OFORMAT_8XP:
        case OFORMAT_8XG:
        case OFORMAT_8XG_AUTO_EXTRACT:
        case OFORMAT_8XP_AUTO_DECOMPRESS:
            ret = output_bin(file->name, file->arr, file->size, fdo);
            break;

        default:
            ret = 1;
            break;
    }

    fclose(fdo);

    return ret;
}
