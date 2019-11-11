#include "ti8x.h"

#include <stdlib.h>

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
