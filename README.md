# convbin [![Windows/Linux/MacOS](https://github.com/mateoconlechuga/convbin/actions/workflows/make.yml/badge.svg)](https://github.com/mateoconlechuga/convbin/actions/workflows/make.yml) [![Coverity Scan](https://scan.coverity.com/projects/23437/badge.svg)](https://scan.coverity.com/projects/mateoconlechuga-convbin) [![Language Grade](https://img.shields.io/lgtm/grade/cpp/g/mateoconlechuga/convbin.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/mateoconlechuga/convbin/context:cpp)

This program is used to convert files to other formats, specifically for the TI84+CE and related calculators.

## Command Line Help

    Usage:
        convbin [options] -j <mode> -k <mode> -i <file> -o <file>

    Required options:
        -i, --input <file>      Input file. Can be specified multiple times,
                                input files are appended in order.
        -o, --output <file>     Output file after conversion.
        -j, --iformat <mode>    Set per-input file format to <mode>.
                                See 'Input formats' below.
                                This should be placed before the input file.
                                The default input format is 'bin'.
        -p, --icompress <mode>  Set per-input file compression <mode>.
                                This should be placed before the input file.
                                The default compression format is 'none'.
                                Supported modes: zx7, none
        -k, --oformat <mode>    Set output file format to <mode>.
                                See 'Output formats' below.
        -n, --name <name>       If converting to a TI file type, sets
                                the on-calc name. For C, Assembly, and ICE
                                outputs, sets the array or label name.

    Optional options:
        -r, --archive           If TI 8x* format, mark as archived.
        -c, --compress <mode>   Compress output using <mode>.
                                Supported modes: zx7, none
        -m, --maxvarsize <size> Sets maximum size of TI 8x* variables.
        -u, --uppercase         If a program, makes on-calc name uppercase.
        -a, --append            Append to output file rather than overwrite.
        -h, --help              Show this screen.
        -v, --version           Show program version.
        -l, --log-level <level> Set program logging level.
                                0=none, 1=error, 2=warning, 3=normal

    Input formats:
        Below is a list of available input formats, listed as
        <mode>: Description.

        bin: Interpret as raw binary.
        csv: Interprets as csv (comma separated values).
        8x: Interprets the TI 8x* data section.

    Output formats:
        Below is a list of available output formats, listed as
        <mode>: Description.

        c: C source.
        asm: Assembly source.
        ice: ICE source.
        bin: raw binary.
        8xp: TI Program.
        8xv: TI Appvar.
        8xg: TI Group. Input format must be 8x.
        8xg-auto-extract: TI Auto-Extracting Group. Input format must be 8x.
        8xp-auto-decompress: TI Auto-Decompressing Compressed Program.

    Credits:
        (c) 2017-2021 by Matt "MateoConLechuga" Waltz.

        This program utilizes the following neat libraries:
            zx7: (c) 2012-2013 by Einar Saukas.
