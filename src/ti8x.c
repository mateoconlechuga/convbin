#include "ti8x.h"

#include <stdlib.h>

const unsigned char ti8x_file_header[10] =
	{ 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };

/*
 * Computes checksum of TI 8x* format files.
 */
unsigned int ti8x_checksum(unsigned char *arr, size_t size)
{
    unsigned int checksum = 0;
	size_t i;

    for (i = 0; i < size; ++i)
    {
        checksum += arr[TI8X_VAR_HEADER + i];
        checksum &= 0xffff;
    }

	return checksum;
}
