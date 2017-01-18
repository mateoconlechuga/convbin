#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>

#include "zx7.h"

#define m8(x) ((x)&255)
#define m16(x) ((x)&65535)

enum {
    UNARCHIVED = 0,
    ARCHIVED = 128,
};

enum {
    TYPE_PRGM = 6,
    TYPE_APPVAR = 21,
};

enum {
    FILE_UNKNOWN,
    FILE_BIN,
    FILE_HEX,
    FILE_8XP,
};

enum {
    HEADER_START = 0x37,
    DATA_START = 0x4A,
    USERMEM_START = 0xD1A881,
};

static char *strduplicate(const char *str) {
    char *dup = malloc(strlen(str) + 1);
    if (dup) { strcpy(dup, str); }
    return dup;
}

/* useful functions */
static uint8_t charToHexDigit(char c_in) {
    int c = toupper(c_in);
    if (c >= 'A') {
        return (c - 'A' + 10);
    } else {
        return (c - '0');
    }
}

static void strtoupper(char *sPtr) {
    while(*sPtr != '\0') {
        if (islower(*sPtr)) {
            *sPtr = toupper(*sPtr);
        }
        ++sPtr;
    }
}

static const char *get_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

static void w24(unsigned int data, uint8_t *loc) {
    loc[0] = m8(data);
    loc[1] = m8(data>>8);
    loc[2] = m8(data>>16);
}
static unsigned int r24(uint8_t *loc) {
    return loc[0] | (loc[1]<<8) | (loc[2]<<16);
}

static uint8_t asm_extractor[] = { 0x21, 0x91, 0xA8, 0xD1, // label to start of data section (offset = 1)
                                   0x11, 0x66, 0x94, 0xD0, 0x01, 0x99, 0x00, 0x00, 0xED, 0xB0, 0xC3, 0x66, 0x94, 0xD0, 0xED, 0x5B, 0x8C, 0x11, 0xD0, 0x21, 0x81, 0xA8, 0xD1, 
                                   0xCD, 0x90, 0x05, 0x02,
                                   0x21, 0x00, 0x00, 0x00, // size of uncompressed data (offset = 32)
                                   0xCD, 0x1C, 0x05, 0x02, 0xDA, 0x68, 0x07, 0x02, 0xED, 0x53, 0x8C, 0x11, 0xD0, 0xEB, 0x11, 0x81,
                                   0xA8, 0xD1, 0xCD, 0x14, 0x05, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xCD, 0x98, 0x1F, 0x02, 0xEB, 0x28, 0x08, 0x11, 0x09, 0x00, 0x00, 0x19, 0x5E, 0x19,
                                   0x23,
                                   0x11, 0xAD, 0x00, 0x00, // offset to actual data (offset = 76)
                                   0x19,
                                   0x11, 0x7F, 0xA8, 0xD1, // offset to start (offset = 81)
                                   0xCD, 0xB1, 0x94, 0xD0,
                                   0xC3, 0x7F, 0xA8, 0xD1, // offset to start (offset = 89)
                                   0x3E, 0x80, 0xED, 0xA0, 0xCD, 0xF9,
                                   0x94, 0xD0, 0x30, 0xF8, 0xD5, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0xCD, 0xF9, 0x94, 0xD0, 0x30, 0xF9, 0xD4, 0xF9, 0x94, 0xD0,
                                   0xCB, 0x11, 0xCB, 0x10, 0x38, 0x21, 0x15, 0x20, 0xF3, 0x03, 0x5E, 0x23, 0xCB, 0x23, 0x1C, 0x30, 0x0D, 0x16, 0x10, 0xCD, 0xF9, 0x94, 0xD0, 0xCB,
                                   0x12, 0x30, 0xF8, 0x14, 0xCB, 0x3A, 0xCB, 0x1B, 0xE3, 0xE5, 0xED, 0x52, 0xD1, 0xED, 0xB0, 0xE1, 0x30, 0xBC, 0x87, 0xC0, 0x7E, 0x23, 0x17, 0xC9,
                                 };

enum {
    X_DATA_START = 1,
    X_SIZE = 32,
    X_DATA_OFFSET = 77,
    X_EXTRACT_START = 82,
    X_JUMP_START = 90,
};

int main(int argc, char* argv[]) {
    /* variable declartions */
    FILE *out_file = NULL;
    FILE *in_file = NULL;
    char *in_name = NULL;
    char *out_name = NULL;
    char *prgm_name = NULL;
    char *ext = NULL;
    char *tmp = NULL;

    /* header for TI files */
    uint8_t header[]  = { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };
    uint8_t archived  = UNARCHIVED;
    uint8_t file_type = TYPE_PRGM;
    uint8_t len_high;
    uint8_t len_low;
    uint8_t *output = NULL;

    unsigned int i;
    unsigned int offset;
    unsigned int checksum;
    
    size_t name_length;
    size_t data_size;
    size_t output_size;
    size_t total_size;
    
    int opt;
    int input_type = FILE_HEX;
    
    bool name_override = false;
    bool compress_output = false;
    bool add_extractor = false;
    bool output_bin = false;
    
    long delta;
    
    /* separate ouput a bit */
    fputc('\n', stdout);

    if (argc > 1) {
        while ((opt = getopt(argc, argv, "avbxhcn:")) != -1) {
            switch (opt) {
                case 'a':   /* archive output */
                    archived = ARCHIVED;
                    break;
                case 'v':   /* write output to appvar */
                    file_type = TYPE_APPVAR;
                    break;
                case 'n':   /* specify varname */
                    printf("Varname override: '%s'\n", optarg);
                    name_override = true;
                    prgm_name = strduplicate(optarg);
                    name_length = strlen(optarg);
                    break;
                case 'c':   /* compress input */
                    compress_output = true;
                    break;
                case 'x':   /* create a compressed self extracting file */
                    compress_output = true;
                    add_extractor = true;
                    break;
                case 'b':
                    output_bin = true;
                    break;
                case 'h':   /* show help */
                    goto show_help;
                default:
                    fprintf(stdout, "[err] Unrecognized option: '%s'\n", optarg);
                    goto show_help;
            }
        }
    } else {
show_help:
        fprintf(stdout, "ConvHex Utility v2.0 - by M. Waltz\n\n");
        fprintf(stdout, "Usage:\n\tconvhex [-options] <filename>\n");
        fprintf(stdout, "Options:\n");
        fprintf(stdout, "\ta: Mark output binary as archived (Default is unarchived)\n");
        fprintf(stdout, "\tv: Write output to Appvar (Default is program)\n");
        fprintf(stdout, "\tn: Override varname (example: -n MYPRGM)\n");
        fprintf(stdout, "\tx: Create compressed self extracting file (useful for programs, output written to <filename_>)");
        fprintf(stdout, "\tc: Compress input");
        fprintf(stdout, "\tb: Write output to binary file rather than 8x* file");
        fprintf(stdout, "\th: Show this message\n");
        return 0;
    }
    
    /* get the filenames for both out and in files */
    in_name = malloc(strlen(argv[argc-1])+5);
    strcpy(in_name, argv[argc-1]);

    /* change the extension if it exists; otherwise create a new one */
    ext = strrchr(in_name, '.');
    
    /* assume hex file by default, otherwise assume from extension */
    if (ext == NULL) {
        strcat(in_name, ".hex");
        ext = strrchr(in_name, '.');
    } else {
        if (!strcmp(get_ext(in_name), "bin")) {
            input_type = FILE_BIN;
        } else
        if (!strcmp(get_ext(in_name), "8xp")) {
            input_type = FILE_8XP;
        }
    }

    if (!name_override) {
        /* check if the name is too long */
        tmp = strrchr(in_name, '\\');
        if (tmp != NULL) {
            name_length = ext-tmp-1;
            prgm_name = tmp+1;
        } else {
            tmp = strrchr(in_name, '/');
            if (tmp != NULL) {
                name_length = ext-tmp-1;
                prgm_name = tmp+1;
            } else {
                name_length = ext-in_name;
                prgm_name = in_name;
            }
        }
    }
    if (name_length > 8) {
        name_length = 8;
    }

    /* create the output name */
    if (add_extractor) {
        out_name = malloc(strlen(in_name)+6);
        strcpy(out_name, in_name);
        strcpy(out_name+(ext-in_name), "_.8x");
    } else {
        out_name = malloc(strlen(in_name)+5);
        strcpy(out_name, in_name);
        strcpy(out_name+(ext-in_name), ".8x");
    }

    strcat(out_name, (file_type == TYPE_PRGM) ? "p" : "v");
    
    if (output_bin) {
        strcpy((char*)get_ext(out_name), "bin");
    }
    
    /* print out some debug things */
    printf("Input File: %s\nOutput File: %s\n", in_name, out_name);
    
    /* open the specified files */
    in_file = fopen(in_name, "rb");
    if (!in_file) {
        fprintf(stderr, "[err] Unable to open input file.\n");
        goto err;
    }
    
    /* write program name */
    prgm_name[name_length] = '\0';
    strtoupper(prgm_name);
    
    printf("Calculator Name: %s\n", prgm_name);
    printf("Archived: %s\n", (archived == ARCHIVED) ? "Yes" : "No");
    
    /* allocate space for the output file */
    output = calloc(0x80000, 1);

    /* copy the header to the file buffer */
    for(i=0; i<sizeof(header); ++i) {
        output[i] = header[i];
    }
    
    /* store the program name */
    offset = 0x3C;
    for (i=0; i<name_length; ++i) {
        output[offset++] = prgm_name[i];
    }

    /* locate offset at data region */
    offset = DATA_START;
    
    if (input_type == FILE_HEX) {
        /* parse the Intel Hex file, and store it into the data array */
        do {
            if (fgetc(in_file) == ':') {
                fseek(in_file, 7, SEEK_CUR);
                
                /* only parse data sections */
                if (fgetc(in_file) == '0') {
                    do {
                        int c_high = fgetc(in_file);
                        int c_low = fgetc(in_file);
                        
                        /* if detect a newline, break */
                        if (isxdigit(c_high) && isxdigit(c_low)) {
                            output[offset++] = (charToHexDigit(c_high) << 4) | charToHexDigit(c_low);
                             if (offset > 0x7FFFF) {
                                goto err_to_large;
                            }
                        } else {
                            break;
                        }
                    } while (true);
                    
                    /* back over line checksum */
                    offset--;
                }
            }
        } while(!feof(in_file));
    } else
    
    if (input_type == FILE_BIN) {
        int c;
        while((c = fgetc(in_file)) != EOF) {
            output[offset++] = (uint8_t)c;
            if (offset > 0x7FFFF) {
                goto err_to_large;
            }
        }
    } else

    /* simply copy the 8xp data */
    if (input_type == FILE_8XP) {
        int c;
        fseek(in_file, DATA_START, SEEK_SET);
        while((c = fgetc(in_file)) != EOF) {
            output[offset++] = (uint8_t)c;
            if (offset > 0x7FFFF) {
                goto err_to_large;
            }
        }
        
        /* back over checksum */
        offset -= 2;
    }
    
    total_size = data_size = (offset-DATA_START);
    
    /* compress the output */
    if (compress_output) {
        unsigned int offset_to_start = 0;
        uint8_t *compressed_data;
        size_t compressed_size;
        
        offset = DATA_START;
        
        if (add_extractor) {
            if (output[offset] == 0xEF && output[offset+1] == 0x7B) {
                offset += 2;
                
                /* check if we have more meta information */
                if (output[offset] == 0) {
                    offset++;
                    offset_to_start++;
                }
                /* icon stuff is weird */
                if (output[offset] == 0xC3) {
                    offset++;
                    
                    offset_to_start = output[offset] | (output[offset+1]<<8) | (output[offset+2]<<16);
                    offset_to_start -= USERMEM_START;
                    
                    offset += offset_to_start - (offset - DATA_START) + 2;
                }
            }
            
            offset_to_start = offset - DATA_START;
            data_size -= offset_to_start;
            
            /* write uncompressed size */
            w24(total_size, &asm_extractor[X_SIZE]);
            
            /* fixup offsets */
            w24(r24(&asm_extractor[X_DATA_START])    + offset_to_start, &asm_extractor[X_DATA_START]);
            w24(r24(&asm_extractor[X_DATA_OFFSET])   + offset_to_start, &asm_extractor[X_DATA_OFFSET]);
            w24(r24(&asm_extractor[X_EXTRACT_START]) + offset_to_start, &asm_extractor[X_EXTRACT_START]);
            w24(r24(&asm_extractor[X_JUMP_START])    + offset_to_start, &asm_extractor[X_JUMP_START]);

            memcpy(&output[offset], asm_extractor, sizeof(asm_extractor));
            offset += sizeof(asm_extractor);
        }
        
        compressed_data = compress(optimize(&output[offset], data_size), &output[offset], data_size, &compressed_size, &delta);
        
        memcpy(&output[offset], compressed_data, compressed_size);
        offset += compressed_size;
    }
    
    output[HEADER_START] = 0x0D;        /* nessasary */
    output[0x3B] = file_type;           /* write file type */
    output[0x45] = archived;            /* write archive status */

    data_size = (offset - HEADER_START);
    len_high = m8(data_size>>8);
    len_low = m8(data_size);
    output[0x35] = len_low;
    output[0x36] = len_high;

    data_size = (offset-DATA_START);
    len_high = m8(data_size>>8);
    len_low = m8(data_size);
    output[0x48] = len_low;
    output[0x49] = len_high;

    data_size += 2;               /* for size bytes */
    len_high = m8(data_size>>8);
    len_low = m8(data_size);
    output[0x39] = len_low;
    output[0x3A] = len_high;
    output[0x46] = len_low;
    output[0x47] = len_high;

    /* Calculate checksum */
    checksum = 0;
    for(i=0x37; i<offset; ++i) {
        checksum = m16(checksum + output[i]);
    }

    output[offset++] = m8(checksum);
    output[offset++] = m8(checksum>>8);

    output_size = data_size+name_length+7;

    /* make sure our output file isn't too big */
    if (output_size > 0xFFFF-30) {
        goto err_to_large;
    }

    /* close the input file handler */
    fclose(in_file);
    
    /* write the buffer to the file */
    out_file = fopen(out_name, "wb");
    if (!out_file) {
        fprintf(stderr, "[err] Unable to open output file.\n");
        goto err;
    }
    
    if (output_bin) {
        total_size -= 0x4A+2;
        output_size -= 0x4A+2;
        fwrite(&output[0x4A], 1, offset-0x4A-2, out_file);
    } else {
        fwrite(output, 1, offset, out_file);
    }
    
    /* close the file handlers */
    fclose(out_file);

    /* free the out_name buffer */
    free(in_name);
    free(out_name);
    free(output);
    fprintf(stdout, "Success!\n\n");
    if (compress_output) {
        fprintf(stdout,"Decompressed Size: %u bytes\n", total_size);
    }
    fprintf(stdout, "Output Size: %u bytes\n", output_size);
    if (output_size > total_size) {
        fprintf(stdout,"\n[WARNING] Compressed size larger than input.\n");
    }
    return 0;
    
err_to_large:
    fprintf(stderr, "[err] Input file too large.\n");
err:
    fclose(out_file);
    fclose(in_file);
    free(in_name);
    free(out_name);
    free(output);
    return 1;
}
