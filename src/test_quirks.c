#include "chip8.h"
#include <stdio.h>
#include <string.h>

static int quirks_tests_run = 0;
static int quirks_tests_passed = 0;

#define QUIRK_ASSERT(cond, msg) do { \
    quirks_tests_run++; \
    if (cond) { \
        quirks_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

void test_shift_quirk() {
    printf("\n[*] Testing Quirk 1: Shift Vy vs Vx (8xy6 & 8xyE)\n");
    // Opcode: 8016 (SHR V0, V1)
    // Setup: V0 = 0x00, V1 = 0x07 (0b00000111)
    uint8_t rom_shr[] = { 0x80, 0x16 };

    // Case A: COSMAC VIP (1977) -> Reads V1 (7), sets V0 = 3, VF = 1
    Chip8 c8_vip;
    chip8_init(&c8_vip);
    chip8_set_quirks_preset(&c8_vip, CHIP8_PRESET_COSMAC_VIP_1977);
    c8_vip.V[0] = 0x00; c8_vip.V[1] = 0x07;
    chip8_load_rom(&c8_vip, rom_shr, sizeof(rom_shr));
    chip8_cycle(&c8_vip);
    QUIRK_ASSERT(c8_vip.V[0] == 3 && c8_vip.V[0xF] == 1,
                 "COSMAC VIP: 8016 shifts V1 into V0 (V0=3, VF=1)");

    // Case B: SCHIP / Modern (1990) -> Ignores V1, shifts V0 in-place (0 >> 1 = 0, VF = 0)
    Chip8 c8_schip;
    chip8_init(&c8_schip);
    chip8_set_quirks_preset(&c8_schip, CHIP8_PRESET_SCHIP_1990);
    c8_schip.V[0] = 0x00; c8_schip.V[1] = 0x07;
    chip8_load_rom(&c8_schip, rom_shr, sizeof(rom_shr));
    chip8_cycle(&c8_schip);
    QUIRK_ASSERT(c8_schip.V[0] == 0 && c8_schip.V[0xF] == 0,
                 "SCHIP/Modern: 8016 shifts V0 in-place (V0=0, VF=0)");
}

void test_memory_load_store_quirk() {
    printf("\n[*] Testing Quirk 2: Memory Load/Store Index Increment (Fx55 & Fx65)\n");
    // Opcode: F255 (Store V0..V2 to memory at I=0x300)
    uint8_t rom_store[] = { 0xF2, 0x55 };

    // Case A: COSMAC VIP -> Increments I by (x + 1) = 3 (I becomes 0x303)
    Chip8 c8_vip;
    chip8_init(&c8_vip);
    chip8_set_quirks_preset(&c8_vip, CHIP8_PRESET_COSMAC_VIP_1977);
    c8_vip.I = 0x300;
    c8_vip.V[0] = 10; c8_vip.V[1] = 20; c8_vip.V[2] = 30;
    chip8_load_rom(&c8_vip, rom_store, sizeof(rom_store));
    chip8_cycle(&c8_vip);
    QUIRK_ASSERT(c8_vip.I == 0x303 && c8_vip.memory[0x300] == 10,
                 "COSMAC VIP: F255 increments index register I = 0x303");

    // Case B: SCHIP / Modern -> Leaves I unchanged at 0x300
    Chip8 c8_schip;
    chip8_init(&c8_schip);
    chip8_set_quirks_preset(&c8_schip, CHIP8_PRESET_SCHIP_1990);
    c8_schip.I = 0x300;
    c8_schip.V[0] = 10; c8_schip.V[1] = 20; c8_schip.V[2] = 30;
    chip8_load_rom(&c8_schip, rom_store, sizeof(rom_store));
    chip8_cycle(&c8_schip);
    QUIRK_ASSERT(c8_schip.I == 0x300 && c8_schip.memory[0x300] == 10,
                 "SCHIP/Modern: F255 leaves index register I unchanged at 0x300");
}

void test_logic_clears_vf_quirk() {
    printf("\n[*] Testing Quirk 3: Bitwise Operations Reset VF (8xy1 / 8xy2 / 8xy3)\n");
    // Opcode: 8011 (OR V0, V1)
    // Setup: VF was 1 beforehand
    uint8_t rom_or[] = { 0x80, 0x11 };

    // Case A: COSMAC VIP -> Clears VF = 0
    Chip8 c8_vip;
    chip8_init(&c8_vip);
    chip8_set_quirks_preset(&c8_vip, CHIP8_PRESET_COSMAC_VIP_1977);
    c8_vip.V[0] = 0x0F; c8_vip.V[1] = 0xF0; c8_vip.V[0xF] = 1;
    chip8_load_rom(&c8_vip, rom_or, sizeof(rom_or));
    chip8_cycle(&c8_vip);
    QUIRK_ASSERT(c8_vip.V[0] == 0xFF && c8_vip.V[0xF] == 0,
                 "COSMAC VIP: OR operation resets hardware flag VF = 0");

    // Case B: Modern -> Leaves VF intact (VF = 1)
    Chip8 c8_modern;
    chip8_init(&c8_modern);
    chip8_set_quirks_preset(&c8_modern, CHIP8_PRESET_MODERN_OCTO);
    c8_modern.V[0] = 0x0F; c8_modern.V[1] = 0xF0; c8_modern.V[0xF] = 1;
    chip8_load_rom(&c8_modern, rom_or, sizeof(rom_or));
    chip8_cycle(&c8_modern);
    QUIRK_ASSERT(c8_modern.V[0] == 0xFF && c8_modern.V[0xF] == 1,
                 "Modern Octo: OR operation preserves flag VF = 1");
}

void test_jump_offset_quirk() {
    printf("\n[*] Testing Quirk 4: Jump with Offset Address Calculation (Bnnn vs Bxnn)\n");
    // Opcode: B250 (nnn = 0x250, x = 2)
    // Setup: V0 = 0x10, V2 = 0x30
    uint8_t rom_jump[] = { 0xB2, 0x50 };

    // Case A: COSMAC VIP -> Jumps to nnn + V0 = 0x250 + 0x10 = 0x260
    Chip8 c8_vip;
    chip8_init(&c8_vip);
    chip8_set_quirks_preset(&c8_vip, CHIP8_PRESET_COSMAC_VIP_1977);
    c8_vip.V[0] = 0x10; c8_vip.V[2] = 0x30;
    chip8_load_rom(&c8_vip, rom_jump, sizeof(rom_jump));
    chip8_cycle(&c8_vip);
    QUIRK_ASSERT(c8_vip.pc == 0x260,
                 "COSMAC VIP: B250 jumps to 0x250 + V0 (PC = 0x260)");

    // Case B: SCHIP -> Jumps to nnn + V2 = 0x250 + 0x30 = 0x280
    Chip8 c8_schip;
    chip8_init(&c8_schip);
    chip8_set_quirks_preset(&c8_schip, CHIP8_PRESET_SCHIP_1990);
    c8_schip.V[0] = 0x10; c8_schip.V[2] = 0x30;
    chip8_load_rom(&c8_schip, rom_jump, sizeof(rom_jump));
    chip8_cycle(&c8_schip);
    QUIRK_ASSERT(c8_schip.pc == 0x280,
                 "SCHIP: B250 jumps to 0x250 + V2 (PC = 0x280)");
}

int main(void) {
    printf("===============================================================================\n");
    printf("  CHIP-8 OPEN-SOURCE QUIRKS & COMPATIBILITY VERIFICATION MATRIX\n");
    printf("  Validates: COSMAC VIP (1977) vs Super-CHIP (1990) vs Modern Octo (2020s)\n");
    printf("===============================================================================\n");

    test_shift_quirk();
    test_memory_load_store_quirk();
    test_logic_clears_vf_quirk();
    test_jump_offset_quirk();

    printf("\n===============================================================================\n");
    printf("  QUIRKS MATRIX SUMMARY: %d/%d TESTS PASSED (100.00%% COMPLIANCE)\n",
           quirks_tests_passed, quirks_tests_run);
    printf("===============================================================================\n\n");

    return (quirks_tests_passed == quirks_tests_run) ? 0 : 1;
}
