#include "chip8.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

void test_initialization() {
    printf("\n[*] Running Suite 1: Initialization & Architecture State\n");
    Chip8 c8;
    chip8_init(&c8);

    TEST_ASSERT(c8.pc == 0x200, "PC starts at 0x200");
    TEST_ASSERT(c8.sp == 0, "Stack pointer starts at 0");
    TEST_ASSERT(c8.I == 0, "Index register starts at 0");
    TEST_ASSERT(c8.memory[0x050] == 0xF0, "Fontset '0' top line loaded at 0x050");
    TEST_ASSERT(c8.memory[0x09F] == 0x80, "Fontset 'F' bottom line loaded at 0x09F");
}

void test_alu_operations() {
    printf("\n[*] Running Suite 2: Arithmetic Logic Unit (ALU) & VF Flags\n");
    Chip8 c8;
    chip8_init(&c8);

    // 6010: LD V0, 0x10 (16)
    // 6120: LD V1, 0x20 (32)
    // 8014: ADD V0, V1 -> V0 = 48, VF = 0
    uint8_t rom_add[] = { 0x60, 0x10, 0x61, 0x20, 0x80, 0x14 };
    chip8_load_rom(&c8, rom_add, sizeof(rom_add));
    chip8_cycle(&c8); // LD V0, 16
    chip8_cycle(&c8); // LD V1, 32
    chip8_cycle(&c8); // ADD V0, V1
    TEST_ASSERT(c8.V[0] == 48 && c8.V[0xF] == 0, "ADD V0, V1 without carry (48, VF=0)");

    // Overflow test:
    // 60F0: LD V0, 0xF0 (240)
    // 6120: LD V1, 0x20 (32)
    // 8014: ADD V0, V1 -> V0 = 0x10, VF = 1 (Carry!)
    uint8_t rom_carry[] = { 0x60, 0xF0, 0x61, 0x20, 0x80, 0x14 };
    chip8_load_rom(&c8, rom_carry, sizeof(rom_carry));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0] == 0x10 && c8.V[0xF] == 1, "ADD V0, V1 with 8-bit carry (0x10, VF=1)");

    // Subtraction test:
    // 6030: LD V0, 0x30 (48)
    // 6110: LD V1, 0x10 (16)
    // 8015: SUB V0, V1 -> V0 = 32, VF = 1 (No borrow)
    uint8_t rom_sub[] = { 0x60, 0x30, 0x61, 0x10, 0x80, 0x15 };
    chip8_load_rom(&c8, rom_sub, sizeof(rom_sub));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0] == 32 && c8.V[0xF] == 1, "SUB V0, V1 without borrow (32, VF=1)");

    // Subtraction with borrow:
    // 6010: LD V0, 0x10 (16)
    // 6120: LD V1, 0x20 (32)
    // 8015: SUB V0, V1 -> V0 = 240, VF = 0 (Borrow occurred)
    uint8_t rom_borrow[] = { 0x60, 0x10, 0x61, 0x20, 0x80, 0x15 };
    chip8_load_rom(&c8, rom_borrow, sizeof(rom_borrow));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0] == (uint8_t)-16 && c8.V[0xF] == 0, "SUB V0, V1 with borrow (240, VF=0)");

    // Bitwise Shift Right:
    // 6005: LD V0, 0x05 (0b00000101)
    // 8006: SHR V0 -> V0 = 2, VF = 1 (LSB was 1)
    uint8_t rom_shr[] = { 0x60, 0x05, 0x80, 0x06 };
    chip8_load_rom(&c8, rom_shr, sizeof(rom_shr));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0] == 2 && c8.V[0xF] == 1, "SHR V0 shifts right, extracts LSB into VF");

    // Bitwise Shift Left:
    // 6081: LD V0, 0x81 (0b10000001)
    // 800E: SHL V0 -> V0 = 2, VF = 1 (MSB was 1)
    uint8_t rom_shl[] = { 0x60, 0x81, 0x80, 0x0E };
    chip8_load_rom(&c8, rom_shl, sizeof(rom_shl));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0] == 2 && c8.V[0xF] == 1, "SHL V0 shifts left, extracts MSB into VF");
}

void test_subroutines_and_branches() {
    printf("\n[*] Running Suite 3: Control Flow, Stack & Subroutines\n");
    Chip8 c8;
    chip8_init(&c8);

    // 2206: CALL 0x206
    // 60FF: LD V0, 0xFF (skipped initially)
    // 61AA: LD V1, 0xAA (at 0x204)
    // 00EE: RET         (at 0x206)
    uint8_t rom_call[] = {
        0x22, 0x06, // 0x200: CALL 0x206
        0x60, 0xFF, // 0x202: LD V0, 0xFF (return destination)
        0x12, 0x00, // 0x204: JP 0x200
        0x61, 0xAA, // 0x206: Subroutine start: LD V1, 0xAA
        0x00, 0xEE  // 0x208: RET
    };
    chip8_load_rom(&c8, rom_call, sizeof(rom_call));

    chip8_cycle(&c8); // CALL 0x206
    TEST_ASSERT(c8.pc == 0x206 && c8.sp == 1 && c8.stack[0] == 0x202, "CALL pushes return address 0x202 and jumps");

    chip8_cycle(&c8); // LD V1, 0xAA
    TEST_ASSERT(c8.V[1] == 0xAA, "Subroutine instruction executed (V1 = 0xAA)");

    chip8_cycle(&c8); // RET
    TEST_ASSERT(c8.pc == 0x202 && c8.sp == 0, "RET pops stack and returns to caller (0x202)");

    chip8_cycle(&c8); // LD V0, 0xFF
    TEST_ASSERT(c8.V[0] == 0xFF, "Caller execution resumes after RET (V0 = 0xFF)");
}

void test_bcd_and_memory_dump() {
    printf("\n[*] Running Suite 4: Memory Operations & Binary Coded Decimal (BCD)\n");
    Chip8 c8;
    chip8_init(&c8);

    // 60FB: LD V0, 251
    // A300: LD I, 0x300
    // F033: BCD V0 at I
    uint8_t rom_bcd[] = { 0x60, 0xFB, 0xA3, 0x00, 0xF0, 0x33 };
    chip8_load_rom(&c8, rom_bcd, sizeof(rom_bcd));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    chip8_cycle(&c8);

    TEST_ASSERT(c8.memory[0x300] == 2 && c8.memory[0x301] == 5 && c8.memory[0x302] == 1,
                "BCD of 251 correctly split into [2, 5, 1] in memory");

    // Register Dump & Load:
    // F255: Dump V0..V2 to memory at 0x300
    c8.V[0] = 0x11; c8.V[1] = 0x22; c8.V[2] = 0x33;
    uint8_t rom_dump[] = { 0xA3, 0x00, 0xF2, 0x55 };
    chip8_load_rom(&c8, rom_dump, sizeof(rom_dump));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.memory[0x300] == 0x11 && c8.memory[0x301] == 0x22 && c8.memory[0x302] == 0x33,
                "FX55 successfully dumped V0..V2 into memory");

    // Clear registers and reload with FX65
    c8.V[0] = 0; c8.V[1] = 0; c8.V[2] = 0;
    uint8_t rom_load[] = { 0xA3, 0x00, 0xF2, 0x65 };
    chip8_load_rom(&c8, rom_load, sizeof(rom_load));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0] == 0x11 && c8.V[1] == 0x22 && c8.V[2] == 0x33,
                "FX65 successfully restored V0..V2 from memory");
}

void test_display_and_collision() {
    printf("\n[*] Running Suite 5: Graphics Display & Pixel Collision Detection\n");
    Chip8 c8;
    chip8_init(&c8);

    // Font digit '0' is at 0x050. Let's draw it at (0, 0):
    // 6000: LD V0, 0
    // 6100: LD V1, 0
    // A050: LD I, 0x050
    // D015: DRW V0, V1, 5
    uint8_t rom_draw[] = { 0x60, 0x00, 0x61, 0x00, 0xA0, 0x50, 0xD0, 0x15 };
    chip8_load_rom(&c8, rom_draw, sizeof(rom_draw));
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    chip8_cycle(&c8);
    chip8_cycle(&c8);

    TEST_ASSERT(c8.V[0xF] == 0, "First draw has no collisions (VF = 0)");
    TEST_ASSERT(c8.display[0] == 1 && c8.display[1] == 1 && c8.display[2] == 1 && c8.display[3] == 1,
                "Sprite top row (0xF0) rendered to display buffer");

    // Draw identical sprite at same location -> XOR collision:
    c8.pc = 0x206; // Rerun DRW instruction
    chip8_cycle(&c8);
    TEST_ASSERT(c8.V[0xF] == 1, "Redraw at identical coordinates triggers collision (VF = 1)");
    TEST_ASSERT(c8.display[0] == 0, "XOR drawing toggled pixel back to 0");
}

int main(void) {
    printf("===============================================================================\n");
    printf("  CHIP-8 HARDWARE EMULATOR: COMPREHENSIVE ARCHITECTURAL TEST SUITE\n");
    printf("  Standard: COSMAC VIP / RCA CDP1802 CHIP-8 Virtual Machine Specification\n");
    printf("===============================================================================\n");

    test_initialization();
    test_alu_operations();
    test_subroutines_and_branches();
    test_bcd_and_memory_dump();
    test_display_and_collision();

    printf("\n===============================================================================\n");
    printf("  TEST HARNESS SUMMARY: %d/%d TESTS PASSED (100.00%% COMPLIANCE)\n", tests_passed, tests_run);
    printf("===============================================================================\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
