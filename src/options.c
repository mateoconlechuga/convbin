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

#include "options.h"
#include "output.h"
#include "input.h"
#include "log.h"

#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

static void options_show(const char *prgm)
{
    LOG_PRINT("This program is used to convert files to other formats,\n");
    LOG_PRINT("specifically for the TI-84+CE and related calculators.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Usage:\n");
    LOG_PRINT("    %s [options] -j <mode> -k <mode> -i <file> -o <file>\n", prgm);
    LOG_PRINT("\n");
    LOG_PRINT("Required parameters:\n");
    LOG_PRINT("    -i, --input <file>         Input file. Can be specified multiple times,\n");
    LOG_PRINT("                               input files are appended in order.\n");
    LOG_PRINT("    -o, --output <file>        Output file after converting.\n");
    LOG_PRINT("    -j, --iformat <mode>       Set per-input file format to <mode>.\n");
    LOG_PRINT("                               See 'Input formats' below.\n");
    LOG_PRINT("                               This should be placed before the input file.\n");
    LOG_PRINT("                               The default input format is 'bin'.\n");
    LOG_PRINT("    -p, --icompress <mode>     Set per-input file compression to <mode>.\n");
    LOG_PRINT("                               See 'Compression formats' below.\n");
    LOG_PRINT("                               This should be placed before the input file.\n");
    LOG_PRINT("    -k, --oformat <mode>       Set output file format to <mode>.\n");
    LOG_PRINT("                               See 'Output formats' below.\n");
    LOG_PRINT("    -n, --name <name>          If converting to a TI file type, sets\n");
    LOG_PRINT("                               the on-calc name. For C, Assembly, and ICE\n");
    LOG_PRINT("                               outputs, sets the array or label name.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Optional parameters:\n");
    LOG_PRINT("    -r, --archive              If using the TI 8x* format, mark as archived.\n");
    LOG_PRINT("    -c, --compress <mode>      Compress output using <mode>.\n");
    LOG_PRINT("                               See 'Compression formats' below.\n");
    LOG_PRINT("    -e, --8xp-compress <mode>  Sets the compression mode for compressed 8xp.\n");
    LOG_PRINT("                               Default is 'zx7'.\n");
    LOG_PRINT("    -m, --maxvarsize <size>    Sets maximum size for the TI 8x* variables.\n");
    LOG_PRINT("    -u, --uppercase            If a program, capitalizes the on-calc name.\n");
    LOG_PRINT("    -a, --append               Append to output file rather than overwrite.\n");
    LOG_PRINT("    -h, --help                 Show this screen.\n");
    LOG_PRINT("    -v, --version              Show the program version.\n");
    LOG_PRINT("    -b, --comment              Custom comment for TI 8x* outputs.\n");
    LOG_PRINT("    -l, --log-level <level>    Set program logging level.\n");
    LOG_PRINT("                               0=none, 1=error, 2=warning, 3=normal\n");
    LOG_PRINT("\n");
    LOG_PRINT("Input formats:\n");
    LOG_PRINT("    Below is a list of available input formats, listed as\n");
    LOG_PRINT("    <mode>: <description>\n");
    LOG_PRINT("\n");
    LOG_PRINT("    bin: Interprets as raw binary.\n");
    LOG_PRINT("    csv: Interprets as csv (comma separated values).\n");
    LOG_PRINT("    elf: Interprets as eZ80 ELF object file.\n");
    LOG_PRINT("    8ek: Interprets as TI 8ek application section.\n");
    LOG_PRINT("    8x:  Interprets as TI 8x* data section.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Output formats:\n");
    LOG_PRINT("    Below is a list of available output formats, listed as\n");
    LOG_PRINT("    <mode>: <description>\n");
    LOG_PRINT("\n");
    LOG_PRINT("    c: C source.\n");
    LOG_PRINT("    asm: Assembly source.\n");
    LOG_PRINT("    ice: ICE source.\n");
    LOG_PRINT("    bin: raw binary.\n");
    LOG_PRINT("    8xp: TI Program.\n");
    LOG_PRINT("    8ek: TI Application.\n");
    LOG_PRINT("    8xv: TI AppVar.\n");
    LOG_PRINT("    8xv-split: Split input across multiple TI Appvars.\n");
    LOG_PRINT("    8xp-compressed: Compressed TI Program.\n");
    LOG_PRINT("    8xg: TI Group. Input format must be 8x.\n");
    LOG_PRINT("    8xg-auto-extract: TI Auto-Extracting Group. Input format must be 8x.\n");
    LOG_PRINT("    b83: Pack input files into TI-83 Premium CE bundle.\n");
    LOG_PRINT("    b84: Pack input files into TI-84 Plus CE bundle.\n");
    LOG_PRINT("    zip: Pack input files into zip archive.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Compression formats:\n");
    LOG_PRINT("    Below is a list of available compression formats, listed as\n");
    LOG_PRINT("    <mode>: <description>\n");
    LOG_PRINT("\n");
    LOG_PRINT("    zx0: ZX0 Compression.\n");
    LOG_PRINT("    zx7: ZX7 Compression.\n");
    LOG_PRINT("    auto: Tries all compression modes to find the best one.\n");
    LOG_PRINT("\n");
    LOG_PRINT("Credits:\n");
    LOG_PRINT("    (c) 2017-2026 by Matt \"MateoConLechuga\" Waltz.\n");
    LOG_PRINT("\n");
    LOG_PRINT("    This program utilizes the following neat libraries:\n");
    LOG_PRINT("        zx0,zx7: (c) 2012-2022 by Einar Saukas.\n");
    LOG_PRINT("        miniz: (c) 2010-2014 by Rich Geldreich.\n");
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
    else if (!strcmp(str, "elf"))
    {
        format = IFORMAT_ELF;
    }
    else if (!strcmp(str, "8ek"))
    {
        format = IFORMAT_TI8EK;
    }
    else
    {
        format = IFORMAT_INVALID;
    }

    return format;
}

static compress_mode_t options_parse_compression(const char *str)
{
    compress_mode_t compress;

    if (!strcmp(str, "zx7"))
    {
        compress = COMPRESS_ZX7;
    }
    else if (!strcmp(str, "zx0"))
    {
        compress = COMPRESS_ZX0;
    }
    else if (!strcmp(str, "auto"))
    {
        compress = COMPRESS_AUTO;
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
    else if (!strcmp(str, "8xv-split"))
    {
        format = OFORMAT_8XV_SPLIT;
    }
    else if (!strcmp(str, "8xg"))
    {
        format = OFORMAT_8XG;
    }
    else if (!strcmp(str, "8ek"))
    {
        format = OFORMAT_8EK;
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
    else if (!strcmp(str, "b83"))
    {
        format = OFORMAT_B83;
    }
    else if (!strcmp(str, "b84"))
    {
        format = OFORMAT_B84;
    }
    else if (!strcmp(str, "zip"))
    {
        format = OFORMAT_ZIP;
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

static int options_validate_format(const struct options *options)
{
    oformat_t oformat = options->output.file.format;

    if (oformat == OFORMAT_C ||
        oformat == OFORMAT_ASM ||
        oformat == OFORMAT_ICE ||
        oformat == OFORMAT_8XP ||
        oformat == OFORMAT_8XV ||
        oformat == OFORMAT_8XG ||
        oformat == OFORMAT_8EK ||
        oformat == OFORMAT_8XP_COMPRESSED)
    {
        if (options->output.file.var.name[0] == 0)
        {
            LOG_ERROR("Output variable name not supplied.\n");
            return OPTIONS_FAILED;
        }

        if (oformat == OFORMAT_8XP ||
            oformat == OFORMAT_8XV ||
            oformat == OFORMAT_8XG ||
            oformat == OFORMAT_8EK ||
            oformat == OFORMAT_8XP_COMPRESSED)
        {
            if (strlen(options->output.file.var.name) > TI8X_VAR_NAME_LEN)
            {
                LOG_ERROR("Output variable name too long (limited to %u characters).\n", TI8X_VAR_NAME_LEN);
                return OPTIONS_FAILED;
            }
        }
    }

    if (oformat == OFORMAT_8XP ||
        oformat == OFORMAT_8EK ||
        oformat == OFORMAT_8XP_COMPRESSED)
    {
        if (isdigit(options->output.file.var.name[0]))
        {
            LOG_WARNING("Potentially invalid output variable name (starts with digit).\n");
        }
    }

    return OPTIONS_SUCCESS;
}

static int options_validate(const struct options *options)
{
    if (options->input.nr_files == 0)
    {
        LOG_ERROR("Unknown input file(s).\n");
        return OPTIONS_FAILED;
    }

    if (options->input.files[0].format == IFORMAT_INVALID)
    {
        LOG_ERROR("Invalid input format mode.\n");
        return OPTIONS_FAILED;
    }

    if (options->input.files[0].compression == COMPRESS_INVALID)
    {
        LOG_ERROR("Invalid input compression mode.\n");
        return OPTIONS_FAILED;
    }

    if (options->output.file.name == NULL)
    {
        LOG_ERROR("Unknown output file.\n");
        return OPTIONS_FAILED;
    }

    if (options->output.file.format == OFORMAT_INVALID)
    {
        LOG_ERROR("Invalid output format mode.\n");
        return OPTIONS_FAILED;
    }

    if (options->output.file.compression == COMPRESS_INVALID)
    {
        LOG_ERROR("Invalid output compression mode.\n");
        return OPTIONS_FAILED;
    }

    if (options->output.file.var.maxsize < TI8X_MINIMUM_MAXVAR_SIZE)
    {
        LOG_ERROR("Maximum variable size too small.\n");
        return OPTIONS_FAILED;
    }

    if (options->output.file.var.maxsize > TI8X_MAXDATA_SIZE)
    {
        LOG_ERROR("Maximum variable size too large.\n");
        return OPTIONS_FAILED;
    }

    return options_validate_format(options);
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
    options->output.file.var.namelen = 0;
    options->output.file.size = 0;
    options->output.file.compressed_size = 0;
    options->output.file.uncompressed_size = 0;
    options->output.file.ti8xp_compression = COMPRESS_ZX7;

    memset(options->output.file.var.name, 0, TI8X_VAR_MAX_NAME_LEN + 1);
    memset(options->output.file.comment, 0, MAX_COMMENT_SIZE);
}

static void options_set_comment(struct options *options, const char *comment)
{
    if (comment != NULL)
    {
        size_t size = strlen(comment);

        if (size > MAX_COMMENT_SIZE)
        {
            size = MAX_COMMENT_SIZE;
        }

        memcpy(options->output.file.comment, optarg, size);
    }
}

static void options_configure(struct options *options)
{
    if (options->input.nr_files == 1)
    {
        options->input.files[0].format = options->input.default_format;
    }

    options->output.file.var.type =
        options_get_var_type(options->output.file.format);

    if (options->output.file.format == OFORMAT_8XG ||
        options->output.file.format == OFORMAT_8XG_AUTO_EXTRACT)
    {
        uint32_t i;

        for (i = 0; i < options->input.nr_files; ++i)
        {
            options->input.files[i].format = IFORMAT_TI8X_DATA_VAR;
        }
    }
}

int options_get(int argc, char *argv[], struct options *options)
{
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
            {"comment",      required_argument, 0, 'b'},
            {"archive",      no_argument,       0, 'r'},
            {"uppercase",    no_argument,       0, 'u'},
            {"append",       no_argument,       0, 'a'},
            {"help",         no_argument,       0, 'h'},
            {"version",      no_argument,       0, 'v'},
            {"log-level",    required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "b:e:i:o:j:k:p:c:m:n:l:ruahv", long_options, NULL);
        if (c < 0)
            break;

        switch (c)
        {
            case 'i':
                if (input_add_file_path(&options->input, optarg))
                {
                    return OPTIONS_FAILED;
                }
                break;

            case 'o':
                options->output.file.name = optarg;
                break;

            case 'n':
                output_set_varname(&options->output, optarg);
                break;

            case 'r':
                options->output.file.var.archive = true;
                break;

            case 'j':
                options->input.default_format =
                    options_parse_input_format(optarg);
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

            case 'b':
                options_set_comment(options, optarg);
                break;

            case 'm':
                options->output.file.var.maxsize = strtol(optarg, NULL, 0);
                break;

            case 'v':
                LOG_PRINT(VERSION_STRING "\n");
                return OPTIONS_IGNORE;

            case 'l':
                log_set_level(strtol(optarg, NULL, 0));
                break;

            case 'p':
                options->input.default_compression =
                    options_parse_compression(optarg);
                break;

            case 'h':
                options_show(options->prgm);
                return OPTIONS_IGNORE;

            default:
                options_show(options->prgm);
                return OPTIONS_FAILED;
        }
    }

    options_configure(options);

    return options_validate(options);
}
