# convbin [![linux status](https://travis-ci.org/mateoconlechuga/convhex.svg?branch=master)](https://travis-ci.org/mateoconlechuga/convhex) [![windows status](https://ci.appveyor.com/api/projects/status/jkq338od89qjch4w/branch/master?svg=true)](https://ci.appveyor.com/project/MattWaltz/convbin/branch/master)

This program is used to convert files to other formats, specifically for the TI84+CE and related calculators.

## Command Line Help

    Usage:
        ./convbin [options] -j <mode> -k <mode> -i <file> -o <file>

    Required options:
        -i, --input <file>      Input file. Can be specified multiple times,
                                input files are appended in order.
        -o, --output <file>     Output file after conversion.
        -j, --iformat <mode>    Set input file format to <mode>.
                                See 'Input formats' below.
                                This should be placed before the input file.
                                The default input format is 'bin'.
        -k, --oformat <mode>    Set output file format to <mode>.
                                See 'Output formats' below.
        -n, --name <name>       If converting to a TI file type, sets
                                the on-calc name. For C, Assembly, and ICE
                                outputs, sets the array or label name.

    Optional options:
        -r, --archive           If TI 8x* format, mark as archived.
        -c, --compress <mode>   Compress output using <mode>.
                                Supported modes: zx7
        -m, --maxvarsize <size> Sets maximum size of TI 8x* variables.
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
