/*
 * Copyright 2017-2026 Matt "MateoConLechuga" Waltz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "elf.h"
#include "log.h"
#include "input.h"

#include <string.h>
#include <stdlib.h>

/* ELF32 constants */
#define EI_NIDENT 16
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EM_Z80 220
#define PT_LOAD 1
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_NOBITS 8
#define SHT_RELA 4
#define SHF_ALLOC 0x2
#define SHN_ABS 0xfff1
#define R_Z80_24 5

/* ELF32 structures */
struct elf32_ehdr
{
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf32_shdr
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

struct elf32_phdr
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

struct elf32_rela
{
    uint32_t r_offset;
    uint32_t r_info;
    int32_t r_addend;
};

struct elf32_sym
{
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
};

struct segment_info
{
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t offset;
};

struct section_mapping
{
    uint32_t section_addr;
    uint32_t segment_offset;
    uint32_t section_idx;
    uint32_t section_size;
};

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static int read_ehdr_from_file(FILE *fd, struct elf32_ehdr *ehdr)
{
    uint8_t buf[52];
    const uint8_t *p;

    if (fread(buf, 1, 52, fd) != 52)
    {
        LOG_ERROR("Failed to read ELF header.\n");
        return -1;
    }

    p = buf;
    memcpy(ehdr->e_ident, p, EI_NIDENT);
    p += EI_NIDENT;

    /* Verify magic */
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3)
    {
        LOG_ERROR("Invalid ELF magic number.\n");
        return -1;
    }

    /* Verify 32-bit little-endian */
    if (ehdr->e_ident[4] != ELFCLASS32)
    {
        LOG_ERROR("Only 32-bit ELF files supported.\n");
        return -1;
    }
    if (ehdr->e_ident[5] != ELFDATA2LSB)
    {
        LOG_ERROR("Only little-endian ELF files supported.\n");
        return -1;
    }

    ehdr->e_type = read_u16_le(p);
    p += 2;
    ehdr->e_machine = read_u16_le(p);
    p += 2;
    ehdr->e_version = read_u32_le(p);
    p += 4;
    ehdr->e_entry = read_u32_le(p);
    p += 4;
    ehdr->e_phoff = read_u32_le(p);
    p += 4;
    ehdr->e_shoff = read_u32_le(p);
    p += 4;
    ehdr->e_flags = read_u32_le(p);
    p += 4;
    ehdr->e_ehsize = read_u16_le(p);
    p += 2;
    ehdr->e_phentsize = read_u16_le(p);
    p += 2;
    ehdr->e_phnum = read_u16_le(p);
    p += 2;
    ehdr->e_shentsize = read_u16_le(p);
    p += 2;
    ehdr->e_shnum = read_u16_le(p);
    p += 2;
    ehdr->e_shstrndx = read_u16_le(p);

    return 0;
}

static int read_shdr_from_file(FILE *fd, uint32_t offset, struct elf32_shdr *shdr)
{
    uint8_t buf[40];
    const uint8_t *p;

    if (fseek(fd, offset, SEEK_SET) != 0)
    {
        LOG_ERROR("Failed to seek to section header.\n");
        return -1;
    }

    if (fread(buf, 1, 40, fd) != 40)
    {
        LOG_ERROR("Failed to read section header.\n");
        return -1;
    }

    p = buf;
    shdr->sh_name = read_u32_le(p);
    p += 4;
    shdr->sh_type = read_u32_le(p);
    p += 4;
    shdr->sh_flags = read_u32_le(p);
    p += 4;
    shdr->sh_addr = read_u32_le(p);
    p += 4;
    shdr->sh_offset = read_u32_le(p);
    p += 4;
    shdr->sh_size = read_u32_le(p);
    p += 4;
    shdr->sh_link = read_u32_le(p);
    p += 4;
    shdr->sh_info = read_u32_le(p);
    p += 4;
    shdr->sh_addralign = read_u32_le(p);
    p += 4;
    shdr->sh_entsize = read_u32_le(p);

    return 0;
}

static int read_phdr_from_file(FILE *fd, uint32_t offset, struct elf32_phdr *phdr)
{
    uint8_t buf[32];
    const uint8_t *p;

    if (fseek(fd, offset, SEEK_SET) != 0)
    {
        LOG_ERROR("Failed to seek to program header.\n");
        return -1;
    }

    if (fread(buf, 1, 32, fd) != 32)
    {
        LOG_ERROR("Failed to read program header.\n");
        return -1;
    }

    p = buf;
    phdr->p_type = read_u32_le(p);
    p += 4;
    phdr->p_offset = read_u32_le(p);
    p += 4;
    phdr->p_vaddr = read_u32_le(p);
    p += 4;
    phdr->p_paddr = read_u32_le(p);
    p += 4;
    phdr->p_filesz = read_u32_le(p);
    p += 4;
    phdr->p_memsz = read_u32_le(p);
    p += 4;
    phdr->p_flags = read_u32_le(p);
    p += 4;
    phdr->p_align = read_u32_le(p);

    return 0;
}

static int read_rela(const uint8_t *data, struct elf32_rela *rela)
{
    const uint8_t *p = data;

    rela->r_offset = read_u32_le(p);
    p += 4;
    rela->r_info = read_u32_le(p);
    p += 4;
    rela->r_addend = (int32_t)read_u32_le(p);

    return 0;
}

static int read_sym(const uint8_t *data, struct elf32_sym *sym)
{
    const uint8_t *p = data;

    sym->st_name = read_u32_le(p);
    p += 4;
    sym->st_value = read_u32_le(p);
    p += 4;
    sym->st_size = read_u32_le(p);
    p += 4;
    sym->st_info = *p;
    p += 1;
    sym->st_other = *p;
    p += 1;
    sym->st_shndx = read_u16_le(p);

    return 0;
}

static int segment_compare(const void *a, const void *b)
{
    const struct segment_info *sa = (const struct segment_info *)a;
    const struct segment_info *sb = (const struct segment_info *)b;

    if (sa->paddr < sb->paddr)
        return -1;
    if (sa->paddr > sb->paddr)
        return 1;
    return 0;
}

static int build_section_mapping(FILE *fd, const struct elf32_ehdr *ehdr,
                                 const struct segment_info *segments,
                                 uint32_t num_segments,
                                 uint32_t base_addr,
                                 struct section_mapping **out_mappings,
                                 uint32_t *out_count)
{
    struct section_mapping *mappings;
    uint32_t mapping_count = 0;
    uint32_t i;

    mappings = malloc(ehdr->e_shnum * sizeof(struct section_mapping));
    if (mappings == NULL)
    {
        LOG_ERROR("Failed to allocate section mappings.\n");
        return -1;
    }

    /* Build mapping for each section */
    for (i = 0; i < ehdr->e_shnum; i++)
    {
        struct elf32_shdr shdr;
        uint32_t shdr_offset = ehdr->e_shoff + i * ehdr->e_shentsize;
        uint32_t j;

        if (read_shdr_from_file(fd, shdr_offset, &shdr) < 0)
        {
            continue;
        }

        /* Skip non-allocated sections */
        if (!(shdr.sh_flags & SHF_ALLOC))
        {
            continue;
        }

        /* Find which segment contains this section */
        for (j = 0; j < num_segments; j++)
        {
            uint32_t seg_start = segments[j].vaddr;
            uint32_t seg_end = seg_start + segments[j].memsz;

            /* Check if section address falls within this segment */
            if (shdr.sh_addr >= seg_start && shdr.sh_addr < seg_end)
            {
                uint32_t segment_base_offset = segments[j].paddr - base_addr;

                mappings[mapping_count].section_addr = shdr.sh_addr;
                mappings[mapping_count].segment_offset =
                    segment_base_offset + (shdr.sh_addr - seg_start);
                mappings[mapping_count].section_idx = i;
                mappings[mapping_count].section_size = shdr.sh_size;
                LOG_DEBUG("Section %u at 0x%06X maps to offset 0x%06X (size 0x%X)\n",
                         i, shdr.sh_addr, mappings[mapping_count].segment_offset, shdr.sh_size);
                mapping_count++;
                break;
            }
        }
    }

    *out_mappings = mappings;
    *out_count = mapping_count;
    return 0;
}

static int extract_relocations(FILE *fd, uint8_t *data, size_t data_size, uint32_t base_addr,
                               const struct elf32_ehdr *ehdr,
                               const struct segment_info *segments,
                               uint32_t num_segments,
                               struct app_reloc_table *reloc_table)
{
    uint32_t i;
    struct section_mapping *section_mappings = NULL;
    uint32_t section_mapping_count = 0;
    uint8_t *reloc_data = NULL;
    size_t reloc_count = 0;
    int ret = -1;

    if (reloc_table == NULL)
    {
        return 0;
    }

    reloc_table->data = NULL;
    reloc_table->size = 0;

    if (build_section_mapping(fd, ehdr, segments, num_segments, base_addr,
                             &section_mappings, &section_mapping_count) < 0)
    {
        return -1;
    }

    reloc_data = malloc(MAX_APP_RELOC_SIZE);
    if (reloc_data == NULL)
    {
        LOG_ERROR("Failed to allocate relocation table.\n");
        free(section_mappings);
        return -1;
    }

    for (i = 0; i < ehdr->e_shnum; i++)
    {
        struct elf32_shdr shdr;
        struct elf32_shdr target_shdr;
        uint32_t shdr_offset = ehdr->e_shoff + i * ehdr->e_shentsize;
        uint8_t *rela_data;
        uint8_t *symtab_data = NULL;
        struct elf32_shdr symtab_shdr;
        uint32_t target_section_offset = 0;
        uint32_t j;
        int found_mapping = 0;

        if (read_shdr_from_file(fd, shdr_offset, &shdr) < 0)
        {
            continue;
        }

        if (shdr.sh_type != SHT_RELA)
        {
            continue;
        }

        LOG_DEBUG("reloc_sec=%u link=%u info=%u off=0x%X size=0x%X ent=0x%X\n",
                 i, shdr.sh_link, shdr.sh_info, shdr.sh_offset, shdr.sh_size, shdr.sh_entsize);

        if (shdr.sh_info >= ehdr->e_shnum)
        {
            LOG_ERROR("Invalid target section index in relocation section.\n");
            goto cleanup;
        }

        if (read_shdr_from_file(fd, ehdr->e_shoff + shdr.sh_info * ehdr->e_shentsize, &target_shdr) < 0)
        {
            LOG_ERROR("Failed to read target section header.\n");
            goto cleanup;
        }

        LOG_DEBUG("  reloc_target sec=%u addr=0x%X size=0x%X flags=0x%X\n",
                 shdr.sh_info, target_shdr.sh_addr, target_shdr.sh_size, target_shdr.sh_flags);

        for (j = 0; j < section_mapping_count; j++)
        {
            if (section_mappings[j].section_idx == shdr.sh_info)
            {
                target_section_offset = section_mappings[j].segment_offset;
                found_mapping = 1;
                LOG_DEBUG("  reloc_map sec=%u map_addr=0x%X out_off=0x%06X\n",
                         shdr.sh_info, section_mappings[j].section_addr, target_section_offset);
                break;
            }
        }

        if (!found_mapping)
        {
            LOG_DEBUG("  reloc_skip sec=%u reason=unmapped\n", shdr.sh_info);
            continue;
        }

        if (shdr.sh_link >= ehdr->e_shnum)
        {
            LOG_ERROR("Invalid symbol table index in relocation section.\n");
            goto cleanup;
        }

        if (read_shdr_from_file(fd, ehdr->e_shoff + shdr.sh_link * ehdr->e_shentsize, &symtab_shdr) < 0)
        {
            LOG_ERROR("Failed to read symbol table header.\n");
            goto cleanup;
        }

        if (symtab_shdr.sh_type != SHT_SYMTAB)
        {
            LOG_ERROR("sh_link does not point to a symbol table.\n");
            goto cleanup;
        }

        LOG_DEBUG("  reloc_symtab off=0x%X size=0x%X ent=0x%X\n",
                 symtab_shdr.sh_offset, symtab_shdr.sh_size, symtab_shdr.sh_entsize);

        if (symtab_shdr.sh_entsize == 0 || symtab_shdr.sh_entsize < 16)
        {
            LOG_ERROR("Unsupported symbol table entry size: %u\n",
                     (unsigned int)symtab_shdr.sh_entsize);
            goto cleanup;
        }

        symtab_data = malloc(symtab_shdr.sh_size);
        if (symtab_data == NULL)
        {
            LOG_ERROR("Failed to allocate memory for symbol table.\n");
            goto cleanup;
        }

        if (fseek(fd, symtab_shdr.sh_offset, SEEK_SET) != 0 ||
            fread(symtab_data, 1, symtab_shdr.sh_size, fd) != symtab_shdr.sh_size)
        {
            LOG_ERROR("Failed to read symbol table.\n");
            free(symtab_data);
            goto cleanup;
        }

        /* Read relocation entries */
        rela_data = malloc(shdr.sh_size);
        if (rela_data == NULL)
        {
            LOG_ERROR("Failed to allocate memory for relocation section.\n");
            free(symtab_data);
            goto cleanup;
        }

        if (fseek(fd, shdr.sh_offset, SEEK_SET) != 0 ||
            fread(rela_data, 1, shdr.sh_size, fd) != shdr.sh_size)
        {
            LOG_ERROR("Failed to read relocation section.\n");
            free(rela_data);
            free(symtab_data);
            goto cleanup;
        }

        if (shdr.sh_entsize == 0)
        {
            LOG_ERROR("Relocation section entry size is zero.\n");
            free(rela_data);
            free(symtab_data);
            goto cleanup;
        }

        if (shdr.sh_size % shdr.sh_entsize != 0)
        {
            LOG_ERROR("Relocation section size is not a multiple of entry size.\n");
            free(rela_data);
            free(symtab_data);
            goto cleanup;
        }

        /* Process each relocation entry */
        for (j = 0; j + shdr.sh_entsize <= shdr.sh_size; j += shdr.sh_entsize)
        {
            struct elf32_rela rela;
            struct elf32_sym sym;
            uint32_t r_type;
            uint32_t r_sym;
            uint32_t hole_offset;
            uint32_t reloc_target_value;
            uint32_t unrelocated_value;
            uint8_t *entry;

            read_rela(rela_data + j, &rela);

            r_type = rela.r_info & 0xFF;
            r_sym = rela.r_info >> 8;

            if (r_type != R_Z80_24)
            {
                LOG_ERROR("Unsupported relocation type: %u (expected R_Z80_24)\n", r_type);
                free(rela_data);
                free(symtab_data);
                goto cleanup;
            }

            if (r_sym * symtab_shdr.sh_entsize >= symtab_shdr.sh_size)
            {
                LOG_ERROR("Symbol index %u out of bounds.\n", r_sym);
                free(rela_data);
                free(symtab_data);
                goto cleanup;
            }

            read_sym(symtab_data + r_sym * symtab_shdr.sh_entsize, &sym);

            const char *status = "fix";
            int do_relocate = 1;

            if (sym.st_shndx == SHN_ABS)
            {
                status = "abs";
                do_relocate = 0;
            }

            /* Calculate offset in the binary using section-relative offset */
            if (do_relocate)
            {
                if (rela.r_offset < target_shdr.sh_addr)
                {
                    LOG_ERROR("Relocation offset 0x%06X before section start 0x%06X\n",
                             rela.r_offset, target_shdr.sh_addr);
                    free(rela_data);
                    free(symtab_data);
                    goto cleanup;
                }
                hole_offset = target_section_offset + (rela.r_offset - target_shdr.sh_addr);

                if (hole_offset + 2 >= data_size)
                {
                    LOG_ERROR("Relocation offset 0x%06X out of bounds\n", hole_offset);
                    free(rela_data);
                    free(symtab_data);
                    goto cleanup;
                }
            }
            else
            {
                hole_offset = 0;
            }

            reloc_target_value = sym.st_value + (uint32_t)rela.r_addend;
            unrelocated_value = (reloc_target_value - base_addr) & 0xFFFFFF;

            if (do_relocate && unrelocated_value >= 0x400000)
            {
                status = "range";
                do_relocate = 0;
            }

            LOG_DEBUG("  reloc=%s r_off=0x%06X r_info=0x%08X r_add=%d sym=%u sym_val=0x%06X target=0x%06X unrel=0x%06X hole=0x%06X\n",
                     status, rela.r_offset, rela.r_info, rela.r_addend, r_sym, sym.st_value,
                     reloc_target_value, unrelocated_value, hole_offset);

            if (!do_relocate)
            {
                continue;
            }

            data[hole_offset + 0] = 0xFF;
            data[hole_offset + 1] = 0xFF;
            data[hole_offset + 2] = 0xFF;

            if ((reloc_count + 1) * 6 > MAX_APP_RELOC_SIZE)
            {
                LOG_ERROR("Relocation table exceeded maximum size.\n");
                free(rela_data);
                free(symtab_data);
                goto cleanup;
            }

            entry = reloc_data + reloc_count * 6;
            entry[0] = hole_offset >> 0;
            entry[1] = hole_offset >> 8;
            entry[2] = hole_offset >> 16;
            entry[3] = unrelocated_value;
            entry[4] = unrelocated_value >> 8;
            entry[5] = unrelocated_value >> 16;
            reloc_count++;
        }

        free(rela_data);
        free(symtab_data);
    }

    reloc_table->data = reloc_data;
    reloc_table->size = reloc_count * 6;
    ret = 0;

cleanup:
    free(section_mappings);
    if (ret < 0)
    {
        free(reloc_data);
    }
    return ret;
}

int elf_extract_binary(FILE *fd, uint8_t *data, size_t *size, struct app_reloc_table *reloc_table)
{
    struct elf32_ehdr ehdr;
    struct segment_info *segments = NULL;
    uint32_t num_segments = 0;
    uint32_t min_paddr = 0xFFFFFFFF;
    uint32_t max_paddr = 0;
    uint32_t i;
    int ret = -1;

    if (fd == NULL || data == NULL || size == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'.\n", __func__);
        return -1;
    }

    if (fseek(fd, 0, SEEK_SET) != 0)
    {
        LOG_ERROR("Failed to seek to beginning of file.\n");
        return -1;
    }

    if (read_ehdr_from_file(fd, &ehdr) < 0)
    {
        return -1;
    }

    if (ehdr.e_phoff == 0 || ehdr.e_phnum == 0)
    {
        LOG_ERROR("No program headers in ELF file.\n");
        return -1;
    }

    segments = malloc(ehdr.e_phnum * sizeof(struct segment_info));
    if (segments == NULL)
    {
        LOG_ERROR("Memory allocation failed.\n");
        return -1;
    }

    for (i = 0; i < ehdr.e_phnum; i++)
    {
        struct elf32_phdr phdr;
        uint32_t phdr_offset = ehdr.e_phoff + i * ehdr.e_phentsize;

        if (read_phdr_from_file(fd, phdr_offset, &phdr) < 0)
        {
            goto cleanup;
        }

        if (phdr.p_type == PT_LOAD && phdr.p_filesz > 0)
        {
            uint32_t segment_end;
            
            if (phdr.p_memsz < phdr.p_filesz)
            {
                LOG_ERROR("Segment memsz smaller than filesz at paddr 0x%X.\n",
                         phdr.p_paddr);
                goto cleanup;
            }

            if (phdr.p_vaddr > UINT32_MAX - phdr.p_memsz)
            {
                LOG_ERROR("Segment address overflow at vaddr 0x%X + size 0x%X.\n",
                         phdr.p_vaddr, phdr.p_memsz);
                goto cleanup;
            }

            if (phdr.p_paddr > UINT32_MAX - phdr.p_memsz)
            {
                LOG_ERROR("Segment address overflow at paddr 0x%X + size 0x%X.\n",
                         phdr.p_paddr, phdr.p_memsz);
                goto cleanup;
            }

            segment_end = phdr.p_paddr + phdr.p_memsz;

            segments[num_segments].paddr = phdr.p_paddr;
            segments[num_segments].vaddr = phdr.p_vaddr;
            segments[num_segments].filesz = phdr.p_filesz;
            segments[num_segments].memsz = phdr.p_memsz;
            segments[num_segments].offset = phdr.p_offset;
            num_segments++;

            if (phdr.p_paddr < min_paddr)
                min_paddr = phdr.p_paddr;
            if (segment_end > max_paddr)
                max_paddr = segment_end;
        }
    }

    if (num_segments == 0)
    {
        LOG_ERROR("No loadable segments found.\n");
        goto cleanup;
    }

    qsort(segments, num_segments, sizeof(struct segment_info), segment_compare);

    if (max_paddr < min_paddr)
    {
        LOG_ERROR("Invalid segment address range (max < min).\n");
        goto cleanup;
    }

    *size = max_paddr - min_paddr;
    if (*size > INPUT_MAX_SIZE)
    {
        LOG_ERROR("Output too large: %u bytes.\n", (unsigned int)*size);
        goto cleanup;
    }

    memset(data, 0, *size);

    for (i = 0; i < num_segments; i++)
    {
        uint32_t dest_offset = segments[i].paddr - min_paddr;

        if (dest_offset + segments[i].filesz > *size)
        {
            LOG_ERROR("Segment data exceeds output buffer bounds.\n");
            goto cleanup;
        }

        if (fseek(fd, segments[i].offset, SEEK_SET) != 0)
        {
            LOG_ERROR("Failed to seek to segment data.\n");
            goto cleanup;
        }

        if (fread(data + dest_offset, 1, segments[i].filesz, fd) != segments[i].filesz)
        {
            LOG_ERROR("Failed to read segment data.\n");
            goto cleanup;
        }
    }

    if (extract_relocations(fd, data, *size, min_paddr, &ehdr, segments, num_segments, reloc_table) < 0)
    {
        goto cleanup;
    }

    ret = 0;

cleanup:
    free(segments);
    return ret;
}
