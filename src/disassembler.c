#include "disassembler.h"
#include <stdio.h>
#include <string.h>

void chip8_disassemble_instruction(uint16_t addr, uint16_t opcode, char *out_buf, size_t buf_size) {
    uint8_t  nibble1 = (opcode & 0xF000) >> 12;
    uint8_t  x       = (opcode & 0x0F00) >> 8;
    uint8_t  y       = (opcode & 0x00F0) >> 4;
    uint8_t  nibble4 = (opcode & 0x000F);
    uint8_t  kk      = (opcode & 0x00FF);
    uint16_t nnn     = (opcode & 0x0FFF);

    switch (nibble1) {
        case 0x0:
            if (opcode == 0x00E0) snprintf(out_buf, buf_size, "0x%04X: %04X    CLS", addr, opcode);
            else if (opcode == 0x00EE) snprintf(out_buf, buf_size, "0x%04X: %04X    RET", addr, opcode);
            else snprintf(out_buf, buf_size, "0x%04X: %04X    SYS 0x%03X", addr, opcode, nnn);
            break;

        case 0x1: snprintf(out_buf, buf_size, "0x%04X: %04X    JP  0x%03X", addr, opcode, nnn); break;
        case 0x2: snprintf(out_buf, buf_size, "0x%04X: %04X    CALL 0x%03X", addr, opcode, nnn); break;
        case 0x3: snprintf(out_buf, buf_size, "0x%04X: %04X    SE  V%X, 0x%02X", addr, opcode, x, kk); break;
        case 0x4: snprintf(out_buf, buf_size, "0x%04X: %04X    SNE V%X, 0x%02X", addr, opcode, x, kk); break;
        case 0x5: snprintf(out_buf, buf_size, "0x%04X: %04X    SE  V%X, V%X", addr, opcode, x, y); break;
        case 0x6: snprintf(out_buf, buf_size, "0x%04X: %04X    LD  V%X, 0x%02X", addr, opcode, x, kk); break;
        case 0x7: snprintf(out_buf, buf_size, "0x%04X: %04X    ADD V%X, 0x%02X", addr, opcode, x, kk); break;

        case 0x8:
            switch (nibble4) {
                case 0x0: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   V%X, V%X", addr, opcode, x, y); break;
                case 0x1: snprintf(out_buf, buf_size, "0x%04X: %04X    OR   V%X, V%X", addr, opcode, x, y); break;
                case 0x2: snprintf(out_buf, buf_size, "0x%04X: %04X    AND  V%X, V%X", addr, opcode, x, y); break;
                case 0x3: snprintf(out_buf, buf_size, "0x%04X: %04X    XOR  V%X, V%X", addr, opcode, x, y); break;
                case 0x4: snprintf(out_buf, buf_size, "0x%04X: %04X    ADD  V%X, V%X (VF=carry)", addr, opcode, x, y); break;
                case 0x5: snprintf(out_buf, buf_size, "0x%04X: %04X    SUB  V%X, V%X (VF=borrow)", addr, opcode, x, y); break;
                case 0x6: snprintf(out_buf, buf_size, "0x%04X: %04X    SHR  V%X {, V%X}", addr, opcode, x, y); break;
                case 0x7: snprintf(out_buf, buf_size, "0x%04X: %04X    SUBN V%X, V%X (VF=borrow)", addr, opcode, x, y); break;
                case 0xE: snprintf(out_buf, buf_size, "0x%04X: %04X    SHL  V%X {, V%X}", addr, opcode, x, y); break;
                default:  snprintf(out_buf, buf_size, "0x%04X: %04X    DATA 0x%04X", addr, opcode, opcode); break;
            }
            break;

        case 0x9: snprintf(out_buf, buf_size, "0x%04X: %04X    SNE V%X, V%X", addr, opcode, x, y); break;
        case 0xA: snprintf(out_buf, buf_size, "0x%04X: %04X    LD  I, 0x%03X", addr, opcode, nnn); break;
        case 0xB: snprintf(out_buf, buf_size, "0x%04X: %04X    JP  V0, 0x%03X", addr, opcode, nnn); break;
        case 0xC: snprintf(out_buf, buf_size, "0x%04X: %04X    RND V%X, 0x%02X", addr, opcode, x, kk); break;
        case 0xD: snprintf(out_buf, buf_size, "0x%04X: %04X    DRW V%X, V%X, %u", addr, opcode, x, y, nibble4); break;

        case 0xE:
            if (kk == 0x9E) snprintf(out_buf, buf_size, "0x%04X: %04X    SKP  V%X", addr, opcode, x);
            else if (kk == 0xA1) snprintf(out_buf, buf_size, "0x%04X: %04X    SKNP V%X", addr, opcode, x);
            else snprintf(out_buf, buf_size, "0x%04X: %04X    DATA 0x%04X", addr, opcode, opcode);
            break;

        case 0xF:
            switch (kk) {
                case 0x07: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   V%X, DT", addr, opcode, x); break;
                case 0x0A: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   V%X, K", addr, opcode, x); break;
                case 0x15: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   DT, V%X", addr, opcode, x); break;
                case 0x18: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   ST, V%X", addr, opcode, x); break;
                case 0x1E: snprintf(out_buf, buf_size, "0x%04X: %04X    ADD  I, V%X", addr, opcode, x); break;
                case 0x29: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   F, V%X", addr, opcode, x); break;
                case 0x33: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   B, V%X", addr, opcode, x); break;
                case 0x55: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   [I], V%X", addr, opcode, x); break;
                case 0x65: snprintf(out_buf, buf_size, "0x%04X: %04X    LD   V%X, [I]", addr, opcode, x); break;
                default:   snprintf(out_buf, buf_size, "0x%04X: %04X    DATA 0x%04X", addr, opcode, opcode); break;
            }
            break;

        default:
            snprintf(out_buf, buf_size, "0x%04X: %04X    DATA 0x%04X", addr, opcode, opcode);
            break;
    }
}

void chip8_disassemble_rom(const uint8_t *rom_data, size_t rom_size, uint16_t start_addr) {
    printf("\n=== CHIP-8 STATIC DISASSEMBLY & CODE STREAM ANALYSIS ===\n");
    printf("Base Address: 0x%04X | Total Bytes: %zu\n", start_addr, rom_size);
    printf("--------------------------------------------------------\n");

    char line[128];
    for (size_t i = 0; i < rom_size; i += 2) {
        if (i + 1 >= rom_size) {
            printf("0x%04X: %02X00    DATA 0x%02X\n", (uint16_t)(start_addr + i), rom_data[i], rom_data[i]);
            break;
        }
        uint16_t opcode = (uint16_t)((rom_data[i] << 8) | rom_data[i + 1]);
        chip8_disassemble_instruction((uint16_t)(start_addr + i), opcode, line, sizeof(line));
        printf("%s\n", line);
    }
    printf("========================================================\n\n");
}
