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

#include "options.h"
#include "version.h"
#include "log.h"

#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

static void options_show(const char *prgm)
{
    LOG_PRINT("This program is used to convert files to other formats,\n");
    LOG_PRINT("specifically for the TI84+CE and related calculators.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Usage:\n");
    LOG_PRINT("    %s [options] -j <mode> -k <mode> -i <file> -o <file>\n", prgm);
    LOG_PRINT("\n");
    LOG_PRINT("Required options:\n");
    LOG_PRINT("    -i, --input <file>      Input file. Can be specified multiple times,\n");
    LOG_PRINT("                            input files are appended in order.\n");
    LOG_PRINT("    -o, --output <file>     Output file after conversion.\n");
    LOG_PRINT("    -j, --iformat <mode>    Set per-input file format to <mode>.\n");
    LOG_PRINT("                            See 'Input formats' below.\n");
    LOG_PRINT("                            This should be placed before the input file.\n");
    LOG_PRINT("                            The default input format is 'bin'.\n");
    LOG_PRINT("    -p, --icompress <mode>  Set per-input file compression to <mode>.\n");
    LOG_PRINT("                            See 'Compression formats' below.\n");
    LOG_PRINT("                            This should be placed before the input file.\n");
    LOG_PRINT("    -k, --oformat <mode>    Set output file format to <mode>.\n");
    LOG_PRINT("                            See 'Output formats' below.\n");
    LOG_PRINT("    -n, --name <name>       If converting to a TI file type, sets\n");
    LOG_PRINT("                            the on-calc name. For C, Assembly, and ICE\n");
    LOG_PRINT("                            outputs, sets the array or label name.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Optional options:\n");
    LOG_PRINT("    -r, --archive           If TI 8x* format, mark as archived.\n");
    LOG_PRINT("    -c, --compress <mode>   Compress output using <mode>.\n");
    LOG_PRINT("                            See 'Compression formats' below.\n");
    LOG_PRINT("    -e, --8xp-compress <m>  Sets the compression mode for compressed 8xp.\n");
    LOG_PRINT("                            Default is 'zx7'.\n");
    LOG_PRINT("    -m, --maxvarsize <size> Sets maximum size of TI 8x* variables.\n");
    LOG_PRINT("    -u, --uppercase         If a program, makes on-calc name uppercase.\n");
    LOG_PRINT("    -a, --append            Append to output file rather than overwrite.\n");
    LOG_PRINT("    -h, --help              Show this screen.\n");
    LOG_PRINT("    -v, --version           Show program version.\n");
    LOG_PRINT("    -l, --log-level <level> Set program logging level.\n");
    LOG_PRINT("                            0=none, 1=error, 2=warning, 3=normal\n");
    LOG_PRINT("\n");
    LOG_PRINT("Input formats:\n");
    LOG_PRINT("    Below is a list of available input formats, listed as\n");
    LOG_PRINT("    <mode>: Description.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    bin: Interpret as raw binary.\n");
    LOG_PRINT("    csv: Interprets as csv (comma separated values).\n");
    LOG_PRINT("    8x: Interprets the TI 8x* data section.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Output formats:\n");
    LOG_PRINT("    Below is a list of available output formats, listed as\n");
    LOG_PRINT("    <mode>: Description.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    c: C source.\n");
    LOG_PRINT("    asm: Assembly source.\n");
    LOG_PRINT("    ice: ICE source.\n");
    LOG_PRINT("    bin: raw binary.\n");
    LOG_PRINT("    8xp: TI Program.\n");
    LOG_PRINT("    8xv: TI Appvar.\n");
    LOG_PRINT("    8xg: TI Group. Input format must be 8x.\n");
    LOG_PRINT("    8xg-auto-extract: TI Auto-Extracting Group. Input format must be 8x.\n");
    LOG_PRINT("    8xp-compressed: Compressed TI Program.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Compression Formats:\n");
    LOG_PRINT("    Below is a list of available compression formats, listed as\n");
    LOG_PRINT("    <mode>: Description.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    zx0: ZX0 Compression.\n");
    LOG_PRINT("    zx7: ZX7 Compression.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Credits:\n");
    LOG_PRINT("    (c) 2017-2022 by Matt \"MateoConLechuga\" Waltz.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    This program utilizes the following neat libraries:\n");
    LOG_PRINT("        zx0,zx7: (c) 2012-2021 by Einar Saukas.\n");
}

static iformat_t options_parse_input_format(const char *str)
{
    iformat_t format;

    if (!strcmp(str, "bin"))
    {
        format = IFORMAT_BIN;
    }
    else if (!strcmp(str, "8x"))
    {
        format = IFORMAT_TI8X_DATA;
    }
    else if (!strcmp(str, "csv"))
    {
        format = IFORMAT_CSV;
    }
    else
    {
        format = IFORMAT_INVALID;
    }

    return format;
}

static compression_t options_parse_compression(const char *str)
{
    compression_t compress;

    if (!strcmp(str, "zx7"))
    {
        compress = COMPRESS_ZX7;
    }
    else if (!strcmp(str, "zx0"))
    {
        compress = COMPRESS_ZX0;
    }
    else
    {
        compress = COMPRESS_INVALID;
    }

    return compress;
}

static oformat_t options_parse_output_format(const char *str)
{
    oformat_t format;

    if (!strcmp(str, "c"))
    {
        format = OFORMAT_C;
    }
    else if (!strcmp(str, "asm"))
    {
        format = OFORMAT_ASM;
    }
    else if (!strcmp(str, "ice"))
    {
        format = OFORMAT_ICE;
    }
    else if (!strcmp(str, "bin"))
    {
        format = OFORMAT_BIN;
    }
    else if (!strcmp(str, "8xp"))
    {
        format = OFORMAT_8XP;
    }
    else if (!strcmp(str, "8xv"))
    {
        format = OFORMAT_8XV;
    }
    else if (!strcmp(str, "8xg"))
    {
        format = OFORMAT_8XG;
    }
    else if (!strcmp(str, "8xg-auto-extract"))
    {
        format = OFORMAT_8XG_AUTO_EXTRACT;
    }
    else if (!strcmp(str, "8xp-auto-decompress"))
    {
        format = OFORMAT_8XP_COMPRESSED;
    }
    else if (!strcmp(str, "8xp-compressed"))
    {
        format = OFORMAT_8XP_COMPRESSED;
    }
    else
    {
        format = OFORMAT_INVALID;
    }

    return format;
}

static ti8x_var_type_t options_get_var_type(oformat_t format)
{
    ti8x_var_type_t type;

    switch (format)
    {
        case OFORMAT_8XP:
        case OFORMAT_8XP_COMPRESSED:
            type = TI8X_TYPE_PRGM;
            break;

        case OFORMAT_8XV:
            type = TI8X_TYPE_APPVAR;
            break;

        case OFORMAT_8XG:
        case OFORMAT_8XG_AUTO_EXTRACT:
            type = TI8X_TYPE_GROUP;
            break;

        default:
            type = TI8X_TYPE_UNKNOWN;
            break;
    }

    return type;
}

static int options_verify(struct options *options)
{
    oformat_t oformat = options->output.file.format;

    if (options->input.nr_files == 0)
    {
        LOG_ERROR("Unknown input file(s).\n");
        goto error;
    }

    if (options->input.files[0].format == IFORMAT_INVALID)
    {
        LOG_ERROR("Invalid input format mode.\n");
        goto error;
    }

    if (options->input.files[0].compression == COMPRESS_INVALID)
    {
        LOG_ERROR("Invalid input compression mode.\n");
        goto error;
    }

    if (options->output.file.name == NULL)
    {
        LOG_ERROR("Unknown output file.\n");
        goto error;
    }

    if (options->output.file.format == OFORMAT_INVALID)
    {
        LOG_ERROR("Invalid output format mode.\n");
        goto error;
    }

    if (options->output.file.compression == COMPRESS_INVALID)
    {
        LOG_ERROR("Invalid output compression mode.\n");
        goto error;
    }

    if (options->output.file.var.maxsize < TI8X_MINIMUM_MAXVAR_SIZE)
    {
        LOG_ERROR("Maximum variable size too small.\n");
        goto error;
    }

    if (options->output.file.var.maxsize > TI8X_MAXDATA_SIZE)
    {
        LOG_ERROR("Maximum variable size too large.\n");
        goto error;
    }

    if (oformat == OFORMAT_C ||
        oformat == OFORMAT_ASM ||
        oformat == OFORMAT_ICE ||
        oformat == OFORMAT_8XP ||
        oformat == OFORMAT_8XV ||
        oformat == OFORMAT_8XG ||
        oformat == OFORMAT_8XP_COMPRESSED)
    {
        if (options->output.file.var.name[0] == 0)
        {
            LOG_ERROR("Variable name not supplied.\n");
            goto error;
        }

        if (oformat == OFORMAT_8XP ||
            oformat == OFORMAT_8XV ||
            oformat == OFORMAT_8XG ||
            oformat == OFORMAT_8XP_COMPRESSED)
        {
            if (strlen(options->output.file.var.name) > TI8X_VAR_NAME_LEN)
            {
                LOG_ERROR("Variable name too long.\n");
                goto error;
            }
        }
    }

    if (oformat == OFORMAT_8XP ||
        oformat == OFORMAT_8XP_COMPRESSED)
    {
        if (isdigit(options->output.file.var.name[0]))
        {
            LOG_WARNING("Potentially invalid name (starts with digit)\n");
        }
    }

    return OPTIONS_SUCCESS;

error:
    return OPTIONS_FAILED;
}

static void options_set_default(struct options *options)
{
    options->prgm = 0;
    options->input.nr_files = 0;
    options->input.default_format = IFORMAT_BIN;
    options->input.default_compression = COMPRESS_NONE;
    options->output.file.append = false;
    options->output.file.uppercase = false;
    options->output.file.compression = COMPRESS_NONE;
    options->output.file.name = 0;
    options->output.file.format = OFORMAT_INVALID;
    options->output.file.var.maxsize = TI8X_DEFAULT_MAXVAR_SIZE;
    options->output.file.var.archive = false;
    options->output.file.size = 0;
    options->output.file.compressed_size = 0;
    options->output.file.uncompressed_size = 0;
    options->output.file.ti8xp_compression = COMPRESS_ZX7;

    memset(options->output.file.var.name, 0, TI8X_VAR_MAX_NAME_LEN + 1);
}

int options_get(int argc, char *argv[], struct options *options)
{
    const char *varname = NULL;
    unsigned int nr_files = 0;

    log_set_level(LOG_BUILD_LEVEL);

    if (argc < 2 || argv == NULL || options == NULL)
    {
        options_show(argc < 1 || argv == NULL ? PRGM_NAME : argv[0]);
        return OPTIONS_FAILED;
    }

    options_set_default(options);
    options->prgm = argv[0];

    for (;;)
    {
        int c;
        static struct option long_options[] =
        {
            {"input",        required_argument, 0, 'i'},
            {"output",       required_argument, 0, 'o'},
            {"iformat",      required_argument, 0, 'j'},
            {"oformat",      required_argument, 0, 'k'},
            {"icompress",    required_argument, 0, 'p'},
            {"compress",     required_argument, 0, 'c'},
            {"8xp-compress", required_argument, 0, 'e'},
            {"maxvarsize",   required_argument, 0, 'm'},
            {"name",         required_argument, 0, 'n'},
            {"archive",      no_argument,       0, 'r'},
            {"uppercase",    no_argument,       0, 'u'},
            {"append",       no_argument,       0, 'a'},
            {"help",         no_argument,       0, 'h'},
            {"version",      no_argument,       0, 'v'},
            {"log-level",    required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "e:i:o:j:k:p:c:m:n:l:ruahv", long_options, NULL);
        if (c < 0)
            break;

        switch (c)
        {
            case 'i':
                options->input.files[nr_files].name = optarg;
                options->input.files[nr_files].format =
                    options->input.default_format;
                options->input.files[nr_files].compression =
                    options->input.default_compression;
                nr_files++;
                if (nr_files >= INPUT_MAX_NUM)
                {
                    LOG_ERROR("Too many input files.\n");
                    return OPTIONS_FAILED;
                }
                break;

            case 'o':
                options->output.file.name = optarg;
                break;

            case 'n':
                varname = optarg;
                break;

            case 'r':
                options->output.file.var.archive = true;
                break;

            case 'j':
                options->input.default_format =
                    options_parse_input_format(optarg);
                break;

            case 'p':
                options->input.default_compression =
                    options_parse_compression(optarg);
                break;

            case 'k':
                options->output.file.format =
                    options_parse_output_format(optarg);
                break;

            case 'c':
                options->output.file.compression =
                    options_parse_compression(optarg);
                break;

            case 'e':
                options->output.file.ti8xp_compression =
                    options_parse_compression(optarg);
                break;

            case 'u':
                options->output.file.uppercase = true;
                break;

            case 'a':
                options->output.file.append = true;
                break;

            case 'm':
                options->output.file.var.maxsize =
                    (size_t)strtol(optarg, NULL, 0);
                break;

            case 'v':
                LOG_PRINT("%s v%s by mateoconlechuga\n", PRGM_NAME, VERSION_STRING);
                return OPTIONS_IGNORE;

            case 'l':
                log_set_level((log_level_t)strtol(optarg, NULL, 0));
                break;

            case 'h':
                options_show(options->prgm);
                return OPTIONS_IGNORE;

            default:
                options_show(options->prgm);
                return OPTIONS_FAILED;
        }
    }

    if (varname != NULL)
    {
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
            if (options->output.file.uppercase && isalpha(varname[i]))
            {
                options->output.file.var.name[i] = toupper(varname[i]);
            }
            else
            {
                options->output.file.var.name[i] = varname[i];
            }
        }

        options->output.file.var.name[len] = '\0';
    }

    if (nr_files == 1)
    {
        options->input.files[nr_files - 1].format =
            options->input.default_format;
    }

    options->input.nr_files = nr_files;
    options->output.file.var.type =
        options_get_var_type(options->output.file.format);

    if (options->output.file.format == OFORMAT_8XG ||
        options->output.file.format == OFORMAT_8XG_AUTO_EXTRACT)
    {
        unsigned int i;

        for (i = 0; i < nr_files; ++i)
        {
            options->input.files[i].format = IFORMAT_TI8X_DATA_VAR;
        }
    }

    return options_verify(options);
}
