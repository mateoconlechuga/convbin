# ConvHex
v1.23  
(C) 2015 M. Waltz

Converts from Intel HEX format to TI-z80 usable format.  
Useful for generating programs directly from the output of ZDS  

##### Usage:  
```convhex [-options] <filename>```

Options:   
* a: Mark output binary as archived (Default is unarchived)
* v: Write output to Appvar (Default is program)
* n: Override varname (example: -n MYPRGM)
* h: Show help message
