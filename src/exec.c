#include <exec.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/fcntl.h>
#include <elf.h>
#include <malloc.h>
#include <sys/unistd.h>
#include <string.h>
#include "FreeRTOS.h"

static void log(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    printf("[EXEC] ");
    vprintf(fmt, args);
    putchar('\n');
    va_end(args);
}

#define SYM_NAME_LENGTH 64

union exec_tmp_buffer{
    Elf32_Phdr phdr;
    Elf32_Shdr shdr;
    Elf32_Sym sym;
    Elf32_Ehdr ehdr;
    char name[SYM_NAME_LENGTH];
};

static int seek_read(int fd, off_t offset, void *buf, size_t n){
    lseek(fd, offset, SEEK_SET);
    return read(fd, buf, n);
}

static void seek_strcpy(int fd, off_t offset, char *buf, size_t n){
    lseek(fd, offset, SEEK_SET);
    int i = 0;
    while (i < n-1){
        read(fd, buf+i, 1);
        if(buf[i] == 0){
            break;
        }
        i++;
    }
    buf[i] = 0;
}

#define LOAD_SHDR(i) seek_read(fd, e_shoff + ((i) * sizeof(Elf32_Shdr)), t, sizeof(Elf32_Shdr))

void exec_file(char *name){
    log("Loading %s", name);
    log("Free heap: %04X", xPortGetFreeHeapSize());
    int fd = open(name, O_RDONLY);
    if(fd < 0){
        log("Open failed, abort");
        return;
    }

    union exec_tmp_buffer *t = malloc(sizeof(union exec_tmp_buffer));
    if(!t){
        log("Unable to allocate tmp buffer");
        return;
    }

    seek_read(fd, 0, t, sizeof(Elf32_Ehdr));

    // save necessary values from ehdr
    Elf32_Half e_phnum = t->ehdr.e_phnum;
    Elf32_Off e_phoff = t->ehdr.e_phoff;
    Elf32_Half e_shnum = t->ehdr.e_shnum;
    Elf32_Off e_shoff = t->ehdr.e_shoff;
    Elf32_Addr e_entry = t->ehdr.e_entry;


    size_t image_size = 0;
    // first pass on segment headers
    // to calculate image size
    for(int i = 0; i < e_phnum; i++){
        seek_read(fd, e_phoff + (i * sizeof(Elf32_Phdr)), t, sizeof(Elf32_Phdr));
        if(t->phdr.p_type != PT_LOAD){
            continue;
        }
        size_t segment_end = t->phdr.p_vaddr + t->phdr.p_memsz;
        if(segment_end > image_size){
            image_size = segment_end;
        }
    }


    log("Total image size: %04X", image_size);
    void *image = calloc(image_size, 1);
    if(!image){
        log("Unable to allocate process memory");
        free(t);
        return;
    }
    log("Process memory allocated");

    // second pass on segment headers
    // to load them into memory
    for(int i = 0; i < e_phnum; i++){
        seek_read(fd, e_phoff + (i * sizeof(Elf32_Phdr)), t, sizeof(Elf32_Phdr));
        if(t->phdr.p_type != PT_LOAD){
            continue;
        }

        log("Loading segment %d to [%08x - %08x]", i, image + t->phdr.p_vaddr, image + t->phdr.p_vaddr + t->phdr.p_memsz);
        seek_read(fd, t->phdr.p_offset, image + t->phdr.p_vaddr, t->phdr.p_filesz);
    }

    // find global symbol table and global string table
    Elf32_Off global_syms_offset = 0;
    Elf32_Word global_syms_size = 0;
    Elf32_Off global_strings_offset = 0;
    Elf32_Word global_strings_size = 0;
    for(int i = 0; i < e_shnum; i++){
        LOAD_SHDR(i);
        if(t->shdr.sh_type == SHT_DYNSYM){
            global_syms_offset = t->shdr.sh_offset;
            global_syms_size = t->shdr.sh_size;
            Elf32_Word strings_index = t->shdr.sh_link;
            LOAD_SHDR(strings_index);
            global_strings_offset = t->shdr.sh_offset;
            global_strings_size = t->shdr.sh_size;
        }
    }

    if(!(global_strings_offset && global_syms_offset)){
        log("Cannot find either symbol or strings tables");
        free(t);
        free(image);
        return;
    }

    // do relocations
    for(int sh_num = 0; sh_num < e_shnum; sh_num++){
        LOAD_SHDR(sh_num);
        if(t->shdr.sh_type != SHT_REL){
            continue;
        }
        Elf32_Off rel_offset = t->shdr.sh_offset;
        int rel_count = t->shdr.sh_size / sizeof(Elf32_Rel);
        for(int i = 0; i < rel_count; i++){
            Elf32_Rel rel;
            seek_read(fd, rel_offset + sizeof(Elf32_Rel)*i, &rel, sizeof(Elf32_Rel));
            int sym_num = ELF32_R_SYM(rel.r_info);
            seek_read(fd, global_syms_offset + sym_num*(sizeof(Elf32_Sym)), t, sizeof(Elf32_Sym));
            Elf32_Off sym_name_offset = t->sym.st_name;
            seek_strcpy(fd, global_strings_offset + sym_name_offset, t->name, SYM_NAME_LENGTH);
            log("Relocating %s", t->name);
            size_t sym_addr = find_symbol(t->name);
            if(!sym_addr){
                log("Unable to find symbol %s", t->name);
                free(t);
                free(image);
                return;
            }
            log("Type: %d, Sym addr: %08X, Rel addr: %08X", ELF32_R_TYPE(rel.r_info), sym_addr, image + rel.r_offset);
            switch (ELF32_R_TYPE(rel.r_info)) {
                case R_ARM_JUMP_SLOT:
                case R_ARM_GLOB_DAT:
                    *(Elf32_Word*)(image + rel.r_offset) = sym_addr;
                    break;
                default:
                    log("Unknown relocation type");
                    free(image);
                    free(t);
                    return;
            }

        }
    }



    free(t);
    log("Loading successful, here goes nothing...");
    int (*entry_symbol)(void) = image + e_entry;
    int ret_code = entry_symbol();

    // TODO: exec should not clean up after process,
    // This should be done after all it's tasks had exited
    free(image);
    log("Process exited with code %d", ret_code);
    log("Free heap: %04X", xPortGetFreeHeapSize());
}

size_t find_symbol(char *name){
    size_t symtab_addr = *((size_t *)(0x08000000 + 0x10000 - 8));
    size_t strtab_addr = *((size_t *)(0x08000000 + 0x10000 - 4));

    Elf32_Sym *symtab = (Elf32_Sym *)symtab_addr;
    char *strtab = (char *)strtab_addr;
    uint32_t n_syms = (strtab_addr - symtab_addr) / sizeof(Elf32_Sym);
    
    Elf32_Sym *sym = 0;
    for(int i = 0; i < n_syms; i++){
        if(strcmp(name, strtab + symtab[i].st_name) == 0){
            sym = symtab + i;
            break;
        }
    }
    if(sym == 0){
        return 0;
    }
    return sym->st_value;
}