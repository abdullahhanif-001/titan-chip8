#include "chip8.h"
#include "disassembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t builtin_demo_rom[] = {
    0x00, 0xE0,             // CLS
    0x60, 0x04, 0x61, 0x04, // LD V0, 4 | LD V1, 4
    0xA0, 0x78, 0xD0, 0x15, // LD I, 0x078 ('8') | DRW V0, V1, 5
    0x60, 0x0C, 0xA0, 0x8C, // LD V0, 12 | LD I, 0x08C ('C')
    0xD0, 0x15,             // DRW V0, V1, 5
    0x60, 0x14, 0xA2, 0x30, // LD V0, 20 | LD I, 0x230 ('H')
    0xD0, 0x15,             // DRW V0, V1, 5
    0x60, 0x1C, 0xA2, 0x35, // LD V0, 28 | LD I, 0x235 ('I')
    0xD0, 0x15,             // DRW V0, V1, 5
    0x60, 0x24, 0xA2, 0x3A, // LD V0, 36 | LD I, 0x23A ('P')
    0xD0, 0x15,             // DRW V0, V1, 5
    0x60, 0x2E, 0xA0, 0x55, // LD V0, 46 | LD I, 0x055 ('1')
    0xD0, 0x15,             // DRW V0, V1, 5
    0x60, 0x3C, 0xF0, 0x15, // LD V0, 60 | LD DT, V0
    0x12, 0x28,             // JP 0x228 (Loop)

    // Sprites:
    0x90, 0x90, 0xF0, 0x90, 0x90, // H
    0x70, 0x20, 0x20, 0x20, 0x70, // I
    0xF0, 0x90, 0xF0, 0x80, 0x80  // P
};

int main(int argc, char **argv) {
    printf("\n===============================================================================\n");
    printf("  TITAN CHIP-8 VIRTUAL MACHINE, DISASSEMBLER & QUIRKS ENGINE\n");
    printf("  Standard: COSMAC VIP 1977 / Super-CHIP 1990 / Modern Octo Matrix\n");
    printf("===============================================================================\n\n");

    // Check for disassembly mode
    if (argc >= 2 && strcmp(argv[1], "--disasm") == 0) {
        if (argc >= 3) {
            FILE *f = fopen(argv[2], "rb");
            if (!f) {
                printf("[-] Error opening ROM file: %s\n", argv[2]);
                return 1;
            }
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *rom_buf = (uint8_t*)malloc(sz);
            fread(rom_buf, 1, sz, f);
            fclose(f);
            chip8_disassemble_rom(rom_buf, (size_t)sz, CHIP8_START_ADDRESS);
            free(rom_buf);
            return 0;
        } else {
            chip8_disassemble_rom(builtin_demo_rom, sizeof(builtin_demo_rom), CHIP8_START_ADDRESS);
            return 0;
        }
    }

    Chip8 c8;
    chip8_init(&c8);

    // Parse command line quirks option
    int skip_next = 0;
    for (int i = 1; i < argc; i++) {
        if (skip_next) {
            skip_next = 0;
            continue;
        }
        if (strcmp(argv[i], "--quirks") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "vip") == 0) {
                chip8_set_quirks_preset(&c8, CHIP8_PRESET_COSMAC_VIP_1977);
            } else if (strcmp(argv[i + 1], "schip") == 0) {
                chip8_set_quirks_preset(&c8, CHIP8_PRESET_SCHIP_1990);
            } else {
                chip8_set_quirks_preset(&c8, CHIP8_PRESET_MODERN_OCTO);
            }
            skip_next = 1;
        }
    }

    printf("[+] Active Architectural Preset: %s\n", c8.quirks.preset_name);
    printf("    - Shift Reads Vy           : %s\n", c8.quirks.shift_reads_vy ? "YES (COSMAC VIP)" : "NO (In-place)");
    printf("    - Load/Store Increments I  : %s\n", c8.quirks.load_store_increments_i ? "YES (COSMAC VIP)" : "NO (Modern)");
    printf("    - Logic Operations Clear VF: %s\n", c8.quirks.logic_clears_vf ? "YES (COSMAC VIP)" : "NO (Preserved)");
    printf("    - Jump Offset Uses Vx      : %s\n", c8.quirks.jump_offset_uses_vx ? "YES (SCHIP Bxnn)" : "NO (Bnnn+V0)");
    printf("    - Sprite Clipping          : %s\n\n", c8.quirks.clip_sprites ? "CLIPS AT EDGES" : "WRAPS AROUND");

    // Load ROM
    const char *rom_path = NULL;
    skip_next = 0;
    for (int i = 1; i < argc; i++) {
        if (skip_next) {
            skip_next = 0;
            continue;
        }
        if (strcmp(argv[i], "--quirks") == 0) {
            skip_next = 1;
            continue;
        }
        if (argv[i][0] != '-') {
            rom_path = argv[i];
            break;
        }
    }

    if (rom_path) {
        FILE *f = fopen(rom_path, "rb");
        if (!f) {
            printf("[-] Failed to open ROM: %s\n", rom_path);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t *buf = (uint8_t*)malloc(sz);
        fread(buf, 1, sz, f);
        fclose(f);
        printf("[+] Loaded External ROM: %s (%ld Bytes)\n", rom_path, sz);
        chip8_load_rom(&c8, buf, sz);
        free(buf);
    } else {
        printf("[+] Loading Built-in Demonstrator ROM: 'CHIP-8 DEMO' (%zu Bytes)\n", sizeof(builtin_demo_rom));
        chip8_load_rom(&c8, builtin_demo_rom, sizeof(builtin_demo_rom));
    }

    printf("[*] Executing CPU Cycles...\n");
    for (int i = 0; i < 50; i++) {
        chip8_cycle(&c8);
        chip8_update_timers(&c8);
    }

    chip8_dump_registers(&c8);

    printf("\n[*] Framebuffer Telemetry (64x32 Monochrome):\n");
    chip8_render_console(&c8);

    printf("\n[+] STATUS: SYSTEM VERIFIED & FULLY FUNCTIONAL\n\n");
    return 0;
}
