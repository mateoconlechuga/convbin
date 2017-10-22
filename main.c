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

#define HexFirstCharacter   1
#define HexHeadSize         8
#define HexChecksumSize     2
#define HexStringEnd        2
#define HexLineSize         ((HexFirstCharacter + HexHeadSize + 255 + HexChecksumSize + HexStringEnd) * 2)

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
    ERROR_NONE,
    ERROR_INCOMPATIBLE,
    ERROR_MEMORY,
    ERROR_COMPLETE
};

enum {
    HEADER_START = 0x37,
    DATA_START = 0x4A,
    USERMEM_START = 0xD1A881,
};

typedef enum  {
    HEX_SEGMENT_ADDRESS,
    HEX_LINEAR_ADDRESS
} hex_memtype_t;

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

int8_t asciiToNumeric(int8_t c) {
    if (c >= '0' && c <= '9') {
        return c - 0x30;
    } else if (c >= 'a' && c <= 'f') {
        return c - 87;
    } else if (c >= 'A' && c <= 'F') {
        return c - 55;
    }
    return 0;
}

static void asciiToNumericArray(int8_t *data, int8_t *converted_data, unsigned int data_size) {
	unsigned int pos;

    for (pos = 0; pos < data_size; pos += 2) {
        converted_data[pos/2] = asciiToNumeric(data[pos+1]) + (asciiToNumeric(data[pos]) << 4);
    }
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
    0xEF, 0x7B, 0x21, 0x93, 0xA8, 0xD1, 0x11, 0x00, 0x08, 0xE3, 0x01,
    0xC3, 0x00, 0x00, // size of code + appvars
    0xED, 0xB0,
    0xC3, 0x00, 0x08, 0xE3, 0x21, 0x81, 0xA8, 0xD1, 0xED, 0x5B, 0x8C, 0x11, 0xD0, 0xCD, 0x90, 0x05,
    0x02, 0xAF, 0xED, 0x62, 0x22, 0x8C, 0x11, 0xD0, 0x21, 0xC4, 0x08, 0xE3, 0xBE, 0xCA, 0x83, 0x08,
    0xE3, 0x2B, 0xE5, 0xCD, 0x20, 0x03, 0x02, 0xCD, 0x0C, 0x05, 0x02, 0x30, 0x04, 0xC3, 0x83, 0x08,
    0xE3, 0xCD, 0x98, 0x1F, 0x02, 0x20, 0x0E, 0xCD, 0x28, 0x06, 0x02, 0xCD, 0x48, 0x14, 0x02, 0xCD,
    0xC4, 0x05, 0x02, 0x18, 0xDE, 0xEB, 0x11, 0x09, 0x00, 0x00, 0x19, 0x5E, 0x19, 0x23, 0xCD, 0x9C,
    0x1D, 0x02, 0xE5, 0xD5, 0x11, 0x81, 0xA8, 0xD1, 0x2A, 0x8C, 0x11, 0xD0, 0x19, 0xEB, 0xE1, 0xD5,
    0xE5, 0xCD, 0x14, 0x05, 0x02, 0xE1, 0xE5, 0xED, 0x5B, 0x8C, 0x11, 0xD0, 0x19, 0x22, 0x8C, 0x11,
    0xD0, 0xC1, 0xD1, 0xE1, 0xED, 0xB0, 0xE1, 0x01, 0x0C, 0x00, 0x00, 0x09, 0xAF, 0xBE, 0x2B, 0xC2,
    0x1E, 0x08, 0xE3, 0xC3, 0x81, 0xA8, 0xD1, 0xCD, 0x3C, 0x1A, 0x02, 0xCD, 0x14, 0x08, 0x02, 0xCD,
    0x28, 0x08, 0x02, 0x21, 0xAD, 0x08, 0xE3, 0xCD, 0xC0, 0x07, 0x02, 0xCD, 0x4C, 0x01, 0x02, 0xFE,
    0x09, 0x28, 0x06, 0xFE, 0x0F, 0x28, 0x02, 0x18, 0xF2, 0xCD, 0x14, 0x08, 0x02, 0xC3, 0x28, 0x08,
    0x02, 0x45, 0x52, 0x52, 0x4F, 0x52, 0x3A, 0x20, 0x4D, 0x69, 0x73, 0x73, 0x69, 0x6E, 0x67, 0x20,
    0x41, 0x70, 0x70, 0x76, 0x61, 0x72, 0x00
};

enum {
    X_DATA_START = 1,
    X_SIZE = 39,
    X_DATA_OFFSET = 84,
    X_EXTRACT_START = 89,
    X_JUMP_START = 97,
};

enum {
    L_SIZE = 11,
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
    unsigned int max_var_size = MAX_VAR_SIZE;
    
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
        while ((opt = getopt(argc, argv, "atvbxhcm:n:")) != -1) {
            switch (opt) {
                case 'a':   /* archive output */
                    archived = ARCHIVED;
                    break;
                case 'v':   /* write output to appvar */
                    var_type = TYPE_APPVAR;
                    break;
                case 'm':   /* specify max var size */
                    max_var_size = strtol(optarg, NULL, 10);
                    printf("Maximum variable size: %u bytes\n", max_var_size);
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
        
        int8_t current_line[HexLineSize];
        uint8_t err = ERROR_NONE;
        bool hex_got_addr = false;
        unsigned int bin_pos;
        unsigned int max_offset = 0;
        uint32_t hex_phys_addr = 0;
        uint16_t hex_linear_addr = 0;
        uint16_t hex_seg_addr = 0;
        uint32_t hex_rel_addr = 0;

        hex_memtype_t hex_mem = HEX_LINEAR_ADDRESS;
        
        while (err == ERROR_NONE && fgets((char*)current_line, HexLineSize, in_file)) {
            unsigned int line_length;
            int8_t *converted_line;
            
            /* check start code */
            if (current_line[0] != ':') {
                err = ERROR_INCOMPATIBLE;
                break;
            }
            
            /* ensure line is all hex */
            for (line_length = 1; line_length < HexLineSize && current_line[line_length]; line_length++) {
                if (current_line[line_length] == '\r' || current_line[line_length] == '\n') {
                    line_length--;
                    break;
                }
                if (!((current_line[line_length] >= '0' && current_line[line_length] <= '9') ||
                    (current_line[line_length] >= 'a' && current_line[line_length] <= 'f') ||
                    (current_line[line_length] >= 'A' && current_line[line_length] <= 'F'))) {
                    line_length = HexLineSize;
                }
		    }
            if (line_length == HexLineSize ) {
                err = ERROR_INCOMPATIBLE;
                break;
            }
            
            /* allocate the data for converting the line itself from ascii to numeric */
            converted_line = malloc(line_length/2);

		    asciiToNumericArray(&current_line[1], converted_line, line_length);
            
            uint8_t hex_num_bytes = converted_line[0];
            uint16_t hex_addr = ((((uint16_t)converted_line[1]) << 8) & 0xFF00) | (((uint16_t)converted_line[2]) & 0xFF);
            uint8_t hex_type = converted_line[3];
            uint8_t hex_pos = 4;
            
            switch (hex_type) {
                case 0: // data
                    if (hex_mem == HEX_LINEAR_ADDRESS) {
                        hex_phys_addr = ((((uint32_t)hex_linear_addr) << 16) & 0xFFFF0000) | (((uint32_t)hex_addr) & 0xFFFF);
                    } else {
                        hex_phys_addr = ((((uint32_t)hex_seg_addr) << 4) & 0xFFFF0) | (((uint32_t)hex_addr) & 0xFFFF);
                    }
                    if (!hex_got_addr) {
                        hex_rel_addr = hex_phys_addr;
                        hex_got_addr = true;
                    }
                    offset = hex_phys_addr - hex_rel_addr;
                    for (bin_pos = 0; bin_pos < hex_num_bytes; bin_pos++) {
                        data[offset++] = converted_line[hex_pos++];
                    }
                    if (offset > max_offset) {
                        max_offset = offset;
                    }
                    break;
                case 1: // end of file
                    err = ERROR_COMPLETE;
                    break;
                case 2: // extended segment address
                    hex_mem = HEX_SEGMENT_ADDRESS;
                    hex_seg_addr = ((((uint16_t)converted_line[4]) << 8) & 0xFF00) | (((uint16_t)converted_line[5]) & 0xFF);
                    hex_got_addr = false;
                    break;
                case 3: // start segment address
                    break;
                case 4: // extended linear address
                    hex_mem = HEX_LINEAR_ADDRESS;
                    hex_linear_addr = ((((uint16_t)converted_line[4]) << 8) & 0xFF00) | (((uint16_t)converted_line[5]) & 0xFF);
                    hex_got_addr = false;
                    break;
                case 5: // start linear address
                    break;
                default:
                    break;
            }
            
            free(converted_line);
        }
        offset = max_offset;
        if (err != ERROR_COMPLETE) {
            goto err;
        }
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

    if (offset > max_var_size) {    
        if (name_length == 8) {
            printf("error: output name too long for size.\n");
            exit(1);
        }
        
        unsigned int num_appvars = (offset / max_var_size) + 1;
        unsigned int pos = 0;
        unsigned int len = max_var_size;
        
        if (data[0] == 0xEF && data[1] == 0x7B && var_type == TYPE_PRGM) {
            pos = 2;
            len -= 2;
        }
        
        printf("Input data too large (%u bytes); splitting across %u variable(s).\n", offset, num_appvars);
        puts("--------------------");
        
        uint8_t *appvar_info = calloc(0x10000, 1);
        unsigned int appvar_info_pos = sizeof asm_large_extractor;
        
        w24(r24(&asm_large_extractor[L_SIZE]) + (num_appvars * 11) + 2, &asm_large_extractor[L_SIZE]);
        memcpy(appvar_info, asm_large_extractor, appvar_info_pos);
        
        for (unsigned int i = 0; i < num_appvars; i++) {
            char *file_name, *appvar_name;
            char tmpbuf[10];
            
            sprintf(tmpbuf, "appvar%u_", i);
            file_name = str_dupcat(tmpbuf, out_name);
            file_name[strlen(file_name)-1] = 'v';
            sprintf(tmpbuf, "%u", i);
            appvar_name = str_dupcat(var_name, tmpbuf);
            
            export(appvar_name, file_name, data + pos, len, TYPE_APPVAR, ARCHIVED);
            
            appvar_info[appvar_info_pos] = 0x15;
            memcpy(&appvar_info[appvar_info_pos+1], appvar_name, strlen(appvar_name));
            appvar_info_pos += 11;
            
            free(file_name);
            free(appvar_name);
            
            pos += len;
            offset -= len;
            if (offset > max_var_size) {
                len = max_var_size;
            } else {
                len = offset;
            }
        }
        
        if (var_type == TYPE_PRGM) {
            appvar_info[appvar_info_pos] = 0x15; appvar_info_pos += 2;
            export(var_name, out_name, appvar_info, appvar_info_pos, var_type, archived);
        }
        
        free(appvar_info);
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
