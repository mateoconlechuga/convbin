# ConvHex

Converts from Intel HEX format and other formats to TI usable format.
Input extension can be .hex, .bin, .8xp

##### Usage
```convhex [-options] <filename>```

##### Options
* a: Mark output binary as archived (Default is unarchived)
* v: Write output to AppVar (Default is program)
* n: Override varname (example: -n MYPRGM)
* x: Create compressed self extracting file (useful for programs, output written to <filename_>)
* c: Compress input (useful for AppVars)
* b: Write output to binary file rather than 8x* file
* h: Show this message

##### Credits

(C) 2017 Matt Waltz

Uses Einar Saukas's [ZX7 compression library](http://www.worldofspectrum.org/infoseekid.cgi?id=0027996) for optimal compression.
License information can be found in the LICENSE file.
