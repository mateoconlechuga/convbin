# ConvHex

Converts raw binary and hexadecimal formated files to TI-OS readable formats.

### Usage

```convhex [-options] <input file(s)> <output file>```

### Options

* `a`: mark output as archived (default is unarchived)
* `v`: write output to appvar (default is program)
* `x`: create compressed self extracting output for a program
* `c`: compress output (useful for appvars)
* `b`: write output to binary file rather than 8x* file
* `f`: force input file as binary
* `m <size>`: maximum size of expanded appvars
* `n <name>`: override variable name (example: -n MYPRGM)
* `g <num>`: export auto-extracting group (example: -g 2 f1.8xp f2.8xp out.8xg)
* `h`: show this message

### Credits

(C) 2017-2019 Matthew Waltz

Uses Einar Saukas's [ZX7 compression library](http://www.worldofspectrum.org/infoseekid.cgi?id=0027996) for optimal compression.
License information can be found in the LICENSE file.
