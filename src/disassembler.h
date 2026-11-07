#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

void chip8_disassemble_instruction(uint16_t addr, uint16_t opcode, char *out_buf, size_t buf_size);
void chip8_disassemble_rom(const uint8_t *rom_data, size_t rom_size, uint16_t start_addr);

#endif // DISASSEMBLER_H
