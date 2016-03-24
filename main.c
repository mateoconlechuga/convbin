#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#define PGRM_TYPE   0x06
#define APPVAR_TYPE 0x15
#define UNARCHIVED  0x00
#define ARCHIVED    0x80

/* useful functions */
static inline unsigned char charToHexDigit(char c) {
    if (c >= 'A') {
        return (c - 'A' + 10);
    } else {
        return (c - '0');
    }
}

static inline unsigned char ascii2HexToByte(char *ptr) {
    return (charToHexDigit( *ptr )<<4) + charToHexDigit( *(ptr+1) );
}

static inline void str2hex(char *string, unsigned char *result, int len) {
    int i, k=0;

    for( i=0; i<len; i+=2) {
        result[k++] = ascii2HexToByte( string+i );
    }
}

static inline void strtoupper(char *sPtr) {
    while(*sPtr != '\0') {
        if (islower(*sPtr)) {
            *sPtr = toupper(*sPtr);
        }
        ++sPtr;
    }
}

int main(int argc, char* argv[]) {
    /* variable declartions */
    FILE *out_file = NULL;
    FILE *in_file = NULL;
    char *in_name = NULL;
    char *out_name = NULL;
    char *prgm_name = NULL;
    char *ext = NULL;
    char *tmp = NULL;

    /* temporary buffer for reading files */
    char *tmp_buf = NULL;

    /* header for TI files */
    unsigned char header[]  = { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };
    unsigned char archived  = UNARCHIVED;
    unsigned char file_type = PGRM_TYPE;
    unsigned char len_high;
    unsigned char len_low;
    unsigned char *output = NULL;

    int i;
    int opt;
    int offset;
    int checksum;
    int line_size;
    int name_override = 0;
    
    int name_length;
    int data_length;
    int output_size;

    /* separate ouput a bit */
    printf("\n");

    if(argc > 1)
    {
        while ( (opt = getopt(argc, argv, "avhn:") ) != -1)
        {
            switch (opt)
            {
                case 'a':   /* archive output */
                    archived = ARCHIVED;
                    break;
                case 'v':   /* write output to appvar */
                    file_type = APPVAR_TYPE;
                    break;
                case 'n':   /* specify varname */
                    printf("Varname override: '%s'\n", optarg);
                    name_override = 1;
                    prgm_name = strdup(optarg);
                    name_length = strlen(optarg);
                    break;
                case 'h':   /* show help */
                    goto show_help;
                    break;
                default:
                    printf("Unrecognized option: '%s'\n", optarg);
                    goto show_help;
                    break;
            }
        }
    } else {
        show_help:
        printf("ConvHex Utility v1.23 - by M. Waltz\n");
        printf("\nGenerates a formatted TI calculator file from ZDS-generated Intel hex.\n");
        printf("\nUsage:\n\tconvhex [-options] <filename>");
        printf("\nOptions:\n");
        printf("\ta: Mark output binary as archived (Default is unarchived)\n");
        printf("\tv: Write output to Appvar (Default is program)\n");
        printf("\tn: Override varname (example: -n MYPRGM)\n");
        printf("\th: Show this message\n");
        return 1;
    }

    tmp_buf = (char*)malloc(1024);
    
    /* get the filenames for both out and in files */
    in_name = malloc( strlen( argv[argc-1] )+5 );
    strcpy(in_name, argv[argc-1]);

    /* change the extension if it exists; otherwise create a new one */
    ext = strrchr( in_name, '.' );
    if( ext == NULL ) {
        strcat( in_name, ".hex" );
        ext = strrchr( in_name, '.' );
    }

    if (!name_override)
    {
        /* check if the name is too long */
        tmp = strrchr( in_name, '\\' );
        if( tmp != NULL ) {
            name_length = ext-tmp-1;
            prgm_name = tmp+1;
        } else {
            tmp = strrchr( in_name, '/' );
            if( tmp != NULL ) {
                name_length = ext-tmp-1;
                prgm_name = tmp+1;
            } else {
                name_length = ext-in_name;
                prgm_name = in_name;
            }
        }
    }
    if( name_length > 8 ) {
        name_length = 8;
    }

    /* create the output name */
    out_name = malloc( strlen( in_name )+5 );
    strcpy( out_name, in_name );
    strcpy( out_name+(ext-in_name), ".8x");
    strcat( out_name, (file_type == PGRM_TYPE) ? "p" : "v" );

    /* open the specified files */
    in_file = fopen( in_name, "r" );
    if ( !in_file ) {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        goto err;
    }
    out_file = fopen( out_name, "wb" );
    if ( !out_file ) {
        fprintf(stderr, "ERROR: Unable to open output file.\n");
        goto err;
    }

    /* allocate space for the file */
    output = (unsigned char*)calloc( 0x10100, 1 );

    /* copy the header to the file buffer */
    for( i=0; i<10; ++i) {
        output[i] = header[i];
    }

    /* print out some debug things */
    printf("Input File: %s\nOutput File: %s\n", in_name, out_name);

    /* Write program name */
    *(prgm_name+name_length) = '\0';
    strtoupper(prgm_name);

    /* print out some more debug things */
    printf("Output Calculator Name: %s\n", prgm_name);
    printf("Mark archived: %s\n", (archived == ARCHIVED) ? "Yes" : "No");

    /* store the program name */
    offset = 0x3C;
    for ( i=0; i<(int)name_length; ++i) {
        output[offset++] = prgm_name[i];
    }

    /* parse the Intel Hex file, and store it into the data array */
    offset = 0x4A;
    do {
	/* note fgets() basically can be used to get each line really quick */
	fgets( tmp_buf, 1023, in_file );
	
	if( tmp_buf[0] != ':' ) {
	    fprintf(stderr, "ERROR: Invalid Intel Hex format.\n");
	    goto err;
	}
	
	/* only parse data sections */
	if( tmp_buf[8] == '0' ) {
	    /* get the size of the line to convert */
	    line_size = ascii2HexToByte( tmp_buf+1 );

	    str2hex( tmp_buf+9, output+offset, line_size<<1 );
	    offset += line_size;
	}
    } while( !feof( in_file ) );

    output[0x37] = 0x0D;        /* nessasary */
    output[0x3B] = file_type;   /* write file type */
    output[0x45] = archived;    /* write archive status */

    data_length = (offset - 0x37);
    len_high = (data_length>>8)&0xFF;
    len_low = (data_length&0xFF);
    output[0x35] = len_low;
    output[0x36] = len_high;

    data_length = (offset-0x4A);
    len_high = (data_length>>8)&0xFF;
    len_low = (data_length&0xFF);
    output[0x48] = len_low;
    output[0x49] = len_high;

    data_length += 2;       /* for size bytes */
    len_high = (data_length>>8)&0xFF;
    len_low = (data_length&0xFF);
    output[0x39] = len_low;
    output[0x3A] = len_high;
    output[0x46] = len_low;
    output[0x47] = len_high;

    /* Calculate checksum */
    checksum = 0;
    for( i=0x37; i<offset; ++i) {
        checksum = (checksum + output[i])&0xFFFF;
    }

    output[offset++] = (checksum&0xFF);
    output[offset++] = (checksum>>8)&0xFF;

    output_size = data_length+name_length+7;

    /* make sure our output file isn't too big */
    if(output_size > 0xFFFF-30) {
        fprintf(stderr, "ERROR: Input file too large.");
        goto err;
    }

    /* write the buffer to the file */
    fwrite( output, 1, offset, out_file );

    /* close the file handlers */
    fclose( out_file );
    fclose( in_file );

    /* free the out_name buffer */
    free( in_name );
    free( out_name );
    free( output );
    free( tmp_buf );
    printf("Success!\n\nProgram Size: %d bytes\n", output_size);
    return 0;
err:
    fclose( out_file );
    fclose( in_file );
    free( in_name );
    free( out_name );
    free( output );
    free( tmp_buf );
    return 1;
}