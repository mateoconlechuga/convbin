# convbin [![linux status](https://travis-ci.org/mateoconlechuga/convhex.svg?branch=master)](https://travis-ci.org/mateoconlechuga/convhex) [![windows status](https://ci.appveyor.com/api/projects/status/ufus9k2qyy6o5a8p/branch/master?svg=true)](https://ci.appveyor.com/project/MattWaltz/convhex/branch/master)

This program is used to convert files to other formats, specifically for the TI84+CE and related calculators.

## Command Line Help

    Usage:
        convbin [options] -j <mode> -k <mode> -i <file> -o <file>

    Required options:
        -i, --input <file>        Input file. Can be specified multiple times,
                                  input files are appended in order.
        -o, --output <file>       Output file after conversion.
        -j, --iformat <mode>      Set input file format to <mode>.
                                  See 'Input formats' below.
                                  If input has multiple files, place this before
                                  to change the interpreted format for the file.
        -k, --oformat <mode>      Set output file format to <mode>.
                                  See 'Output formats' below.
        -n, --name <name>         If converting to a TI file type, overrides
                                  the  on-calc name.

    Optional options:
        -c, --compress <mode>   Compress output using <mode>.
                                Supported modes: zx7
        -a, --append            Append to output file rather than overwrite.
        -h, --help              Show this screen.
        -v, --version           Show program version.
        -l, --log-level <level> Set program logging level.
                                0=none, 1=error, 2=warning, 3=normal

    Input formats:
        Below is a list of available input formats, listed as
        <mode>: Description.

        bin: Interpret as raw binary.
        hex: Interpret as Intel Hex.
        8x: Interpret as TI 8x* formatted. Only the data section is used.

    Output formats:
        Below is a list of available output formats, listed as
        <mode>: Description.

        c: Output C source.
        asm: Output Assembly source.
        ice: Output ICE source.
        bin: Output raw binary.
        8xp: Output TI Program.
        8xv: Output TI Appvar.
        8xg: Output TI Group.
        8xg-auto-extract: Output TI Auto-Extracting Group.
        8xp-auto-decompress: Output TI Auto-Decompressing Compressed Program.
