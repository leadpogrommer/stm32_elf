#!/usr/bin/env python3

import os
import sys
import struct
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection, StringTableSection
        

rom_origin = 0x08000000
rom_size = 512 * (2**10)

def main():
    input_file = sys.argv[1]
    output_file = sys.argv[2]

    os.system(f'arm-none-eabi-objcopy -O binary {input_file} {output_file}')

    elf = ELFFile(open(input_file, 'rb'))

    section_names = ['.symtab', '.strtab']
    blobs = []
    for name in section_names:
        section: Section = elf.get_section_by_name(name)
        if section is None:
            raise Exception(f'Section {name} not found in input file')
        blobs.append(section.data())
    
    rom_data = open(output_file, 'rb').read(128*(2**10))
    int_size = 4
    if len(rom_data) + sum([len(b) for b in blobs]) + int_size*len(blobs) > rom_size:
        raise Exception("Rom size overflow")
    

    section_pointers:list[int] = []
    for blob in blobs:
        section_pointers.append(rom_origin + len(rom_data))
        rom_data += blob
    
    rom_data = bytearray(rom_data)
    
    pointers = bytearray()
    for ptr in section_pointers:
        pointers += ptr.to_bytes(4, 'little', signed=False)
    
    rom_data += bytes([0]*(rom_size - len(rom_data) - len(pointers)))
    rom_data += pointers

    open(output_file, 'wb').write(rom_data)

    os.system(f"openocd -f ../stm32f103c8_blue_pill.cfg -c 'program  {output_file} preverify verify 0x08000000 ' -c shutdown")
    
# openocd -f ../stm32f103c8_blue_pill.cfg -c 'init' -c 'flash verify_image blink.rom 0x08000000 bin' -c shutdown



if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    os.chdir(script_dir)

    main()



