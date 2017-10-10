#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>

#include "zx7/zx7.h"

#define m8(x)  ((x)&255)
#define mr8(x) (((x)>>8)&255)
#define m16(x) ((x)&65535)

#define MAX_SIZE 0x100000
#define MAX_VAR_SIZE 0xFFFF-30
#define MAX_PRGM_SIZE 0xFFFF-300

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

static char *str_dup(const char *s) {
    char *d = calloc(strlen(s)+1, 1);
    if (d) strcpy(d, s);
    return d;
}

static char *str_dupcat(const char *s, const char *c) {
    if (!s) {
        return str_dup(c);
    } else
    if (!c) {
        return str_dup(s);
    }
    char *d = malloc(strlen(s)+strlen(c)+1);
    if (d) { strcpy(d, s); strcat(d, c); }
    return d;
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

static uint8_t asm_extractor[] = { 
    0x21, 0x91, 0xA8, 0xD1, // label to start of extractor section (offset = 1)
    0x11, 0x66, 0x94, 0xD0, 0x01, 0xA8, 0x00, 0x00, 0xED, 0xB0, 0xC3, 0x66, 0x94, 0xD0, 0xED, 0x5B, 0x8C, 0x11, 0xD0, 0x21, 0x81, 0xA8, 0xD1,
    0xCD, 0x90, 0x05, 0x02, 0xB7, 0xED, 0x62, 0x22, 0x8C, 0x11, 0xD0,
    0x11, 0x00, 0x00, 0x00, // size of uncompressed data (offset = 39)
    0xCD, 0xB8, 0x94, 0xD0, 0xDA, 0x68, 0x07, 0x02, 0xED, 0x53, 0x8C, 0x11, 0xD0, 0xEB, 0x11, 0x81, 0xA8, 0xD1,
    0xCD, 0x14, 0x05, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0xCD, 0x98, 0x1F, 0x02, 0xEB, 0x28, 0x08, 0x11, 0x09, 0x00, 0x00, 0x19, 0x5E, 0x19, 0x23,
    0x11, 0xBC, 0x00, 0x00, // offset to actual data (offset = 84)
    0x19,
    0x11, 0x7F, 0xA8, 0xD1, // offset to start (offset = 89)
    0xCD, 0xC0, 0x94, 0xD0,
    0xC3, 0x7F, 0xA8, 0xD1, // offset to start (offset = 97)
    0xCD, 0xFC, 0x04, 0x02,
    0xB7, 0xED, 0x52, 0xC9, 
    0x3E, 0x80, 0xED, 0xA0, 0xCD, 0x08, 0x95, 0xD0, 0x30, 0xF8, 0xD5, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0xCD, 0x08, 0x95,
    0xD0, 0x30, 0xF9, 0xD4, 0x08, 0x95, 0xD0, 0xCB, 0x11, 0xCB, 0x10, 0x38, 0x21, 0x15, 0x20, 0xF3, 0x03, 0x5E, 0x23, 0xCB, 0x23, 0x1C, 0x30,
    0x0D, 0x16, 0x10, 0xCD, 0x08, 0x95, 0xD0, 0xCB, 0x12, 0x30, 0xF8, 0x14, 0xCB, 0x3A, 0xCB, 0x1B, 0xE3, 0xE5, 0xED, 0x52, 0xD1, 0xED, 0xB0,
    0xE1, 0x30, 0xBC, 0x87, 0xC0, 0x7E, 0x23, 0x17, 0xC9
};

static uint8_t asm_large_extractor[] = {
    0xEF, 0x7B, 0x21, 0x93, 0xA8, 0xD1, 0x11, 0x00, 0x08, 0xE3, 0x01, 0xE8, 0x00, 0x00, 0xED, 0xB0,
    0xC3, 0x00, 0x08, 0xE3, 0xC3, 0x25, 0x08, 0xE3,
    0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // offset to 1st appvar (offset = 25)
    0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // offset to 2nd appvar (offset = 36)
    0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // offset to 3rd appvar (offset = 47)
    0x21, 0x81, 0xA8, 0xD1, 0xED, 0x5B, 0x8C, 0x11, 0xD0, 0xCD, 0x90, 0x05, 0x02, 0xAF, 0xED, 0x62,
    0x22, 0x8C, 0x11, 0xD0, 0x21, 0x05, 0x08, 0xE3, 0xBE, 0xCA, 0xA8, 0x08, 0xE3, 0x2B, 0xE5, 0xCD,
    0x20, 0x03, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0x30, 0x04, 0xC3, 0xA8, 0x08, 0xE3, 0xCD, 0x98, 0x1F,
    0x02, 0x20, 0x0E, 0xCD, 0x28, 0x06, 0x02, 0xCD, 0x48, 0x14, 0x02, 0xCD, 0xC4, 0x05, 0x02, 0x18,
    0xDE, 0xEB, 0x11, 0x09, 0x00, 0x00, 0x19, 0x5E, 0x19, 0x23, 0xCD, 0x9C, 0x1D, 0x02, 0xE5, 0xD5,
    0x11, 0x81, 0xA8, 0xD1, 0x2A, 0x8C, 0x11, 0xD0, 0x19, 0xEB, 0xE1, 0xD5, 0xE5, 0xCD, 0x14, 0x05,
    0x02, 0xE1, 0xE5, 0xED, 0x5B, 0x8C, 0x11, 0xD0, 0x19, 0x22, 0x8C, 0x11, 0xD0, 0xC1, 0xD1, 0xE1,
    0xED, 0xB0, 0xE1, 0x01, 0x0C, 0x00, 0x00, 0x09, 0xAF, 0xBE, 0x2B, 0xC2, 0x43, 0x08, 0xE3, 0xC3,
    0x81, 0xA8, 0xD1, 0xCD, 0x3C, 0x1A, 0x02, 0xCD, 0x14, 0x08, 0x02, 0xCD, 0x28, 0x08, 0x02, 0x21,
    0xD2, 0x08, 0xE3, 0xCD, 0xC0, 0x07, 0x02, 0xCD, 0x4C, 0x01, 0x02, 0xFE, 0x09, 0x28, 0x06, 0xFE,
    0x0F, 0x28, 0x02, 0x18, 0xF2, 0xCD, 0x14, 0x08, 0x02, 0xC3, 0x28, 0x08, 0x02, 0x45, 0x52, 0x52,
    0x4F, 0x52, 0x3A, 0x20, 0x4D, 0x69, 0x73, 0x73, 0x69, 0x6E, 0x67, 0x20, 0x41, 0x70, 0x70, 0x76,
    0x61, 0x72, 0x00
};

enum {
    X_DATA_START = 1,
    X_SIZE = 39,
    X_DATA_OFFSET = 84,
    X_EXTRACT_START = 89,
    X_JUMP_START = 97,
};

bool output_bin = false;

void export(const char *name, const char *file_name, uint8_t *data, size_t size, uint8_t type, uint8_t archived) {
    const uint8_t header[] = { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };
    unsigned int data_size;
    unsigned int i,checksum;
    FILE *out_file;
    
    // gather structure information
    uint8_t *output = calloc(0x10200, sizeof(uint8_t));
    unsigned int offset = size + DATA_START;

    // Write header bytes
    memcpy(output, header, sizeof header);
    
    // write name
    memcpy(&output[0x3C], name, strlen(name));
    memcpy(&output[0x4A], data, size);
    
    // write config bytes
    output[0x37] = 0x0D;
    output[0x3B] = type;
    output[0x45] = archived;

    data_size = offset - HEADER_START;
    output[0x35] = m8(data_size);
    output[0x36] = mr8(data_size);

    data_size = offset - DATA_START;
    output[0x48] = m8(data_size);
    output[0x49] = mr8(data_size);

    // size bytes
    data_size += 2;
    output[0x39] = m8(data_size);
    output[0x3A] = mr8(data_size);
    output[0x46] = m8(data_size);
    output[0x47] = mr8(data_size);

    // calculate checksum
    checksum = 0;
    for (i = HEADER_START; i < offset; ++i) {
        checksum = m16(checksum + output[i]);
    }

    output[offset++] = m8(checksum);
    output[offset++] = mr8(checksum);

    // write the buffer to the file
    if (!(out_file = fopen(file_name, "wb"))) {
        fprintf(stderr, "Unable to open output program file.");
        exit(1);
    }
    
    if (output_bin) {
        fwrite(&output[DATA_START], 1, offset - DATA_START - 2, out_file);
    } else {
        fwrite(output, 1, offset, out_file);
    }
    
    printf("Output File: [%s] %s\n", name, file_name);
    printf("Archived: %s\n", archived == ARCHIVED ? "Yes" : "No");
    printf("Size: %u bytes\n--------------------\n", offset - DATA_START - 2);
    
    // close the file
    fclose(out_file);
    
    // free the memory
    free(output);
}

int main(int argc, char* argv[]) {
    /* variable declartions */
    FILE *in_file = NULL;
    char *in_name = NULL;
    char *out_name = NULL;
    char *var_name = NULL;
    char *ext = NULL;
    char *tmp = NULL;

    /* header for TI files */
    uint8_t archived  = UNARCHIVED;
    uint8_t var_type = TYPE_PRGM;
    uint8_t *data = NULL;
    
    unsigned int offset = 0;
    
    size_t name_length = 0;
    size_t data_size = 0;
    size_t output_size = 0;
    size_t total_size = 0;
    bool compress_output = false;
    
    int opt;
    int input_type = FILE_HEX;
    
    bool name_override = false;
    bool add_extractor = false;

    long delta;
    
    /* separate ouput a bit */
    fputc('\n', stdout);

    if (argc > 1) {
        while ((opt = getopt(argc, argv, "atvbxhcn:")) != -1) {
            switch (opt) {
                case 'a':   /* archive output */
                    archived = ARCHIVED;
                    break;
                case 'v':   /* write output to appvar */
                    var_type = TYPE_APPVAR;
                    break;
                case 'n':   /* specify varname */
                    printf("Varname override: '%s'\n", optarg);
                    name_override = true;
                    var_name = str_dup(optarg);
                    name_length = strlen(optarg);
                    break;
                case 'c':   /* compress input */
                    compress_output = true;
                    break;
                case 'x':   /* create a compressed self extracting file */
                    compress_output = true;
                    add_extractor = true;
                    break;
                case 't':   /* choose smallest output */
                    break;
                case 'b':   /* output a .bin file */
                    output_bin = true;
                    break;
                case 'h':   /* show help */
                    goto show_help;
                default:
                    printf("[error] unrecognized option: '%s'\n", optarg);
                    goto show_help;
            }
        }
    } else {
show_help:
        puts("ConvHex Utility v2.0 - by M. Waltz\n");
        puts("Usage:\n\tconvhex [-options] <filename>");
        puts("Options:");
        puts("\ta: Mark output binary as archived (Default is unarchived)");
        puts("\tv: Write output to AppVar (Default is program)");
        puts("\tn: Override varname (Example: -n MYPRGM)");
        puts("\tx: Create compressed self extracting file\n\t\t(underscore added if compressing 8x*)");
        puts("\tc: Compress input\n\t\t(Useful for AppVars)");
        puts("\tb: Write output to binary file rather than 8x* file");
        puts("\th: Show this message");
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
            var_name = tmp+1;
        } else {
            tmp = strrchr(in_name, '/');
            if (tmp != NULL) {
                name_length = ext-tmp-1;
                var_name = tmp+1;
            } else {
                name_length = ext-in_name;
                var_name = in_name;
            }
        }
    }
    if (name_length > 8) {
        name_length = 8;
    }

    /* create the output name */
    if (add_extractor && input_type == FILE_8XP) {
        out_name = malloc(strlen(in_name)+6);
        strcpy(out_name, in_name);
        strcpy(out_name+(ext-in_name), "_.8x");
    } else {
        out_name = malloc(strlen(in_name)+5);
        strcpy(out_name, in_name);
        strcpy(out_name+(ext-in_name), ".8x");
    }

    strcat(out_name, (var_type == TYPE_PRGM) ? "p" : "v");
    
    if (output_bin) {
        strcpy((char*)get_ext(out_name), "bin");
    }
    
    /* print out some debug things */
    printf("Input File: %s\n", in_name);
    
    /* open the specified files */
    in_file = fopen(in_name, "rb");
    if (!in_file) {
        fprintf(stderr, "[error] unable to open input file.\n");
        goto err;
    }
    
    /* write program name */
    var_name[name_length] = '\0';
    strtoupper(var_name);

    /* set up memory for data section */
    data = calloc(0x100000, 1);
    
    if (input_type == FILE_HEX) {
        /* parse the Intel Hex file, and store it into the data array */
        /* don't bother with too many checks */
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
                            data[offset++] = (charToHexDigit(c_high) << 4) | charToHexDigit(c_low);
                            if (offset > MAX_SIZE-1) {
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
            data[offset++] = (uint8_t)c;
            if (offset > MAX_SIZE-1) {
                goto err_to_large;
            }
        }
    } else

    /* simply copy the 8xp data */
    if (input_type == FILE_8XP) {
        int c;
        fseek(in_file, DATA_START, SEEK_SET);
        while((c = fgetc(in_file)) != EOF) {
            data[offset++] = (uint8_t)c;
            if (offset > MAX_SIZE-1) {
                goto err_to_large;
            }
        }
        
        /* back over checksum */
        offset -= 2;
    }
    
    total_size = data_size = offset;
    
    /* compress the output */
    if (compress_output) {
        unsigned int offset_to_start = 0;
        uint8_t *compressed_data;
        size_t compressed_size;
    
        offset = 0;
        
        if (add_extractor) {
            if (data[0] == 0xEF && data[1] == 0x7B) {
                offset += 2;
                
                /* check if we have more meta information */
                if (data[offset] == 0) {
                    offset++;
                }
                /* icon stuff is weird */
                if (data[offset] == 0xC3) {
                    offset++;
                    
                    offset_to_start = data[offset] | (data[offset+1]<<8) | (data[offset+2]<<16);
                    offset_to_start -= USERMEM_START;
                    
                    offset += offset_to_start - offset + 2;
                }
            }
            
            offset_to_start = offset;
            data_size -= offset;
            
            /* write uncompressed size */
            w24(total_size, &asm_extractor[X_SIZE]);
            
            /* fixup offsets */
            w24(r24(&asm_extractor[X_DATA_START])    + offset_to_start, &asm_extractor[X_DATA_START]);
            w24(r24(&asm_extractor[X_DATA_OFFSET])   + offset_to_start, &asm_extractor[X_DATA_OFFSET]);
            w24(r24(&asm_extractor[X_EXTRACT_START]) + offset_to_start, &asm_extractor[X_EXTRACT_START]);
            w24(r24(&asm_extractor[X_JUMP_START])    + offset_to_start, &asm_extractor[X_JUMP_START]);
        }
        
        Optimal *optimized = optimize(&data[offset], data_size);
        compressed_data = compress(optimized, &data[offset], data_size, &compressed_size, &delta);
        
        if (add_extractor) {
            memcpy(&data[offset], asm_extractor, sizeof asm_extractor);
            offset += sizeof asm_extractor;
        }
        
        memcpy(&data[offset], compressed_data, compressed_size);
        offset += compressed_size;
        
        free(compressed_data);
        free(optimized);
    }

    if (offset > MAX_VAR_SIZE) {    
        if (name_length == 8) {
            printf("error: output name too long for size.\n");
            exit(1);
        }
        
        unsigned int num_appvars = (offset / MAX_VAR_SIZE) + 1;
        unsigned int pos = 0;
        unsigned int len = MAX_VAR_SIZE;
        
        printf("Input data too large (%u bytes); splitting across %u variable(s).\n", offset, num_appvars);
        puts("--------------------");
        
        for (unsigned int i = 0; i < num_appvars; i++) {
            char *file_name, *appvar_name;
            char tmpbuf[10];
            
            sprintf(tmpbuf, "appvar%u_", i);
            file_name = str_dupcat(tmpbuf, out_name);
            file_name[strlen(file_name)-1] = 'v';
            sprintf(tmpbuf, "%u", i);
            appvar_name = str_dupcat(var_name, tmpbuf);
            
            export(appvar_name, file_name, data + pos, len, TYPE_APPVAR, ARCHIVED);
            
            memcpy(&asm_large_extractor[25+(i*11)], appvar_name, strlen(appvar_name));
            
            free(file_name);
            free(appvar_name);
            
            pos += len;
            offset -= len;
            if (offset > MAX_VAR_SIZE) {
                len = MAX_VAR_SIZE;
            } else {
                len = offset;
            }
        }
        
        export(var_name, out_name, asm_large_extractor, sizeof asm_large_extractor, var_type, archived);
    } else {
        puts("--------------------");
        export(var_name, out_name, data, offset, var_type, archived);
    }
    
    if (compress_output) {
        printf("Decompressed Size: %u bytes\n", (unsigned int)total_size);
    }
    if (compress_output && output_size > (total_size+name_length+7)) {
        puts("\n[WARNING] Compressed size larger than input.");
    }
    
    printf("Success!\n\n");
    
    /* close the file handler and buffers */
    goto done;
    
err_to_large:
    fprintf(stderr, "[error] input file too large.\n");
err:
done:
    if (in_file) { fclose(in_file); }
    free(in_name);
    free(out_name);
    free(data);
    return 1;
}
