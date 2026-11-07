#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Standard CHIP-8 80-byte Fontset (Characters 0 through F)
static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void chip8_set_quirks_preset(Chip8 *c8, Chip8Preset preset) {
    if (!c8) return;
    switch (preset) {
        case CHIP8_PRESET_COSMAC_VIP_1977:
            c8->quirks.shift_reads_vy          = true;  // Vx = Vy >> 1
            c8->quirks.load_store_increments_i = true;  // I = I + x + 1
            c8->quirks.logic_clears_vf         = true;  // VF = 0 on AND/OR/XOR
            c8->quirks.jump_offset_uses_vx     = false; // Bnnn jumps to nnn + V0
            c8->quirks.clip_sprites            = true;  // Clips at edge of display
            c8->quirks.preset_name             = "COSMAC VIP (1977 Historical)";
            break;

        case CHIP8_PRESET_SCHIP_1990:
            c8->quirks.shift_reads_vy          = false; // Vx >>= 1 (in-place)
            c8->quirks.load_store_increments_i = false; // I unchanged
            c8->quirks.logic_clears_vf         = false; // VF untouched
            c8->quirks.jump_offset_uses_vx     = true;  // Bxnn jumps to xnn + Vx
            c8->quirks.clip_sprites            = true;  // Clips
            c8->quirks.preset_name             = "Super-CHIP (1990 HP-48)";
            break;

        case CHIP8_PRESET_MODERN_OCTO:
        default:
            c8->quirks.shift_reads_vy          = false; // In-place
            c8->quirks.load_store_increments_i = false; // I unchanged
            c8->quirks.logic_clears_vf         = false; // VF untouched
            c8->quirks.jump_offset_uses_vx     = false; // Jumps to nnn + V0
            c8->quirks.clip_sprites            = false; // Wraps around screen
            c8->quirks.preset_name             = "Modern Octo / XO-CHIP";
            break;
    }
}

void chip8_init(Chip8 *c8) {
    if (!c8) return;
    memset(c8, 0, sizeof(Chip8));

    c8->pc = CHIP8_START_ADDRESS;
    c8->sp = 0;
    c8->I  = 0;
    c8->delay_timer = 0;
    c8->sound_timer = 0;
    c8->draw_flag   = false;

    // Default to Modern Octo preset for broad compatibility
    chip8_set_quirks_preset(c8, CHIP8_PRESET_MODERN_OCTO);

    // Load Fontset into memory starting at 0x050
    memcpy(&c8->memory[CHIP8_FONT_ADDRESS], chip8_fontset, sizeof(chip8_fontset));
    srand((unsigned int)time(NULL));
}

bool chip8_load_rom(Chip8 *c8, const uint8_t *rom_data, size_t rom_size) {
    if (!c8 || !rom_data) return false;
    if (rom_size > (CHIP8_MEMORY_SIZE - CHIP8_START_ADDRESS)) {
        printf("[-] Error: ROM size (%zu bytes) exceeds available memory!\n", rom_size);
        return false;
    }

    memcpy(&c8->memory[CHIP8_START_ADDRESS], rom_data, rom_size);
    c8->pc = CHIP8_START_ADDRESS;
    return true;
}

uint16_t chip8_fetch_opcode(const Chip8 *c8) {
    if (c8->pc >= CHIP8_MEMORY_SIZE - 1) return 0x0000;
    return (uint16_t)((c8->memory[c8->pc] << 8) | c8->memory[c8->pc + 1]);
}

void chip8_update_timers(Chip8 *c8) {
    if (c8->delay_timer > 0) c8->delay_timer--;
    if (c8->sound_timer > 0) c8->sound_timer--;
}

void chip8_cycle(Chip8 *c8) {
    if (!c8) return;

    // 1. Fetch
    uint16_t opcode = chip8_fetch_opcode(c8);
    c8->pc += 2;

    // Decoding components
    uint8_t  nibble1 = (opcode & 0xF000) >> 12;
    uint8_t  x       = (opcode & 0x0F00) >> 8;
    uint8_t  y       = (opcode & 0x00F0) >> 4;
    uint8_t  nibble4 = (opcode & 0x000F);
    uint8_t  kk      = (opcode & 0x00FF);
    uint16_t nnn     = (opcode & 0x0FFF);

    // 2. Decode & Execute
    switch (nibble1) {
        case 0x0:
            if (opcode == 0x00E0) {
                // 00E0: Clear screen
                memset(c8->display, 0, sizeof(c8->display));
                c8->draw_flag = true;
            } else if (opcode == 0x00EE) {
                // 00EE: Return from subroutine
                if (c8->sp > 0) {
                    c8->sp--;
                    c8->pc = c8->stack[c8->sp];
                }
            }
            break;

        case 0x1: // 1nnn: Jump to address nnn
            c8->pc = nnn;
            break;

        case 0x2: // 2nnn: Call subroutine at nnn
            if (c8->sp < CHIP8_STACK_SIZE) {
                c8->stack[c8->sp] = c8->pc;
                c8->sp++;
                c8->pc = nnn;
            }
            break;

        case 0x3: // 3xkk: Skip next instruction if Vx == kk
            if (c8->V[x] == kk) c8->pc += 2;
            break;

        case 0x4: // 4xkk: Skip next instruction if Vx != kk
            if (c8->V[x] != kk) c8->pc += 2;
            break;

        case 0x5: // 5xy0: Skip next instruction if Vx == Vy
            if (nibble4 == 0x0) {
                if (c8->V[x] == c8->V[y]) c8->pc += 2;
            }
            break;

        case 0x6: // 6xkk: Set Vx = kk
            c8->V[x] = kk;
            break;

        case 0x7: // 7xkk: Set Vx = Vx + kk
            c8->V[x] += kk;
            break;

        case 0x8:
            switch (nibble4) {
                case 0x0: // 8xy0: LD Vx, Vy
                    c8->V[x] = c8->V[y];
                    break;
                case 0x1: // 8xy1: OR Vx, Vy
                    c8->V[x] |= c8->V[y];
                    if (c8->quirks.logic_clears_vf) c8->V[0xF] = 0;
                    break;
                case 0x2: // 8xy2: AND Vx, Vy
                    c8->V[x] &= c8->V[y];
                    if (c8->quirks.logic_clears_vf) c8->V[0xF] = 0;
                    break;
                case 0x3: // 8xy3: XOR Vx, Vy
                    c8->V[x] ^= c8->V[y];
                    if (c8->quirks.logic_clears_vf) c8->V[0xF] = 0;
                    break;
                case 0x4: { // 8xy4: ADD Vx, Vy (VF = carry)
                    uint16_t sum = (uint16_t)c8->V[x] + (uint16_t)c8->V[y];
                    uint8_t carry = (sum > 255) ? 1 : 0;
                    c8->V[x] = (uint8_t)(sum & 0xFF);
                    c8->V[0xF] = carry;
                    break;
                }
                case 0x5: { // 8xy5: SUB Vx, Vy (VF = NOT borrow)
                    uint8_t not_borrow = (c8->V[x] >= c8->V[y]) ? 1 : 0;
                    c8->V[x] -= c8->V[y];
                    c8->V[0xF] = not_borrow;
                    break;
                }
                case 0x6: { // 8xy6: SHR Vx {, Vy} (Controlled by shift_reads_vy quirk)
                    uint8_t src = c8->quirks.shift_reads_vy ? c8->V[y] : c8->V[x];
                    uint8_t lsb = src & 0x1;
                    c8->V[x] = src >> 1;
                    c8->V[0xF] = lsb;
                    break;
                }
                case 0x7: { // 8xy7: SUBN Vx, Vy (VF = NOT borrow)
                    uint8_t not_borrow = (c8->V[y] >= c8->V[x]) ? 1 : 0;
                    c8->V[x] = c8->V[y] - c8->V[x];
                    c8->V[0xF] = not_borrow;
                    break;
                }
                case 0xE: { // 8xyE: SHL Vx {, Vy} (Controlled by shift_reads_vy quirk)
                    uint8_t src = c8->quirks.shift_reads_vy ? c8->V[y] : c8->V[x];
                    uint8_t msb = (src & 0x80) ? 1 : 0;
                    c8->V[x] = src << 1;
                    c8->V[0xF] = msb;
                    break;
                }
            }
            break;

        case 0x9: // 9xy0: Skip next instruction if Vx != Vy
            if (nibble4 == 0x0) {
                if (c8->V[x] != c8->V[y]) c8->pc += 2;
            }
            break;

        case 0xA: // Annn: Set I = nnn
            c8->I = nnn;
            break;

        case 0xB: // Bnnn: Jump with offset (Controlled by jump_offset_uses_vx quirk)
            if (c8->quirks.jump_offset_uses_vx) {
                c8->pc = nnn + c8->V[x];
            } else {
                c8->pc = nnn + c8->V[0];
            }
            break;

        case 0xC: // Cxkk: Set Vx = random byte & kk
            c8->V[x] = (uint8_t)((rand() % 256) & kk);
            break;

        case 0xD: { // Dxyn: Draw sprite at (Vx, Vy) with height n (Controlled by clip_sprites quirk)
            uint8_t start_x = c8->V[x] % CHIP8_DISPLAY_WIDTH;
            uint8_t start_y = c8->V[y] % CHIP8_DISPLAY_HEIGHT;
            c8->V[0xF] = 0;

            for (uint8_t row = 0; row < nibble4; row++) {
                if (c8->quirks.clip_sprites && (start_y + row >= CHIP8_DISPLAY_HEIGHT)) {
                    break; // Clip vertically
                }
                uint8_t sprite_byte = c8->memory[c8->I + row];
                for (uint8_t col = 0; col < 8; col++) {
                    if (c8->quirks.clip_sprites && (start_x + col >= CHIP8_DISPLAY_WIDTH)) {
                        break; // Clip horizontally
                    }
                    if ((sprite_byte & (0x80 >> col)) != 0) {
                        uint16_t px, py;
                        if (c8->quirks.clip_sprites) {
                            px = start_x + col;
                            py = start_y + row;
                        } else {
                            px = (start_x + col) % CHIP8_DISPLAY_WIDTH;
                            py = (start_y + row) % CHIP8_DISPLAY_HEIGHT;
                        }
                        uint16_t idx = py * CHIP8_DISPLAY_WIDTH + px;

                        if (c8->display[idx] == 1) {
                            c8->V[0xF] = 1; // Collision detected
                        }
                        c8->display[idx] ^= 1;
                    }
                }
            }
            c8->draw_flag = true;
            break;
        }

        case 0xE:
            if (kk == 0x9E) { // Ex9E: Skip if key in Vx is pressed
                if (c8->V[x] < CHIP8_KEYPAD_SIZE && c8->keypad[c8->V[x]] != 0) {
                    c8->pc += 2;
                }
            } else if (kk == 0xA1) { // ExA1: Skip if key in Vx is NOT pressed
                if (c8->V[x] < CHIP8_KEYPAD_SIZE && c8->keypad[c8->V[x]] == 0) {
                    c8->pc += 2;
                }
            }
            break;

        case 0xF:
            switch (kk) {
                case 0x07: // Fx07: Set Vx = delay timer
                    c8->V[x] = c8->delay_timer;
                    break;
                case 0x0A: { // Fx0A: Wait for key press and store in Vx
                    bool key_pressed = false;
                    for (uint8_t k = 0; k < CHIP8_KEYPAD_SIZE; k++) {
                        if (c8->keypad[k] != 0) {
                            c8->V[x] = k;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (!key_pressed) {
                        c8->pc -= 2; // Repeat instruction until key press
                    }
                    break;
                }
                case 0x15: // Fx15: Set delay timer = Vx
                    c8->delay_timer = c8->V[x];
                    break;
                case 0x18: // Fx18: Set sound timer = Vx
                    c8->sound_timer = c8->V[x];
                    break;
                case 0x1E: // Fx1E: Set I = I + Vx
                    c8->I += c8->V[x];
                    break;
                case 0x29: // Fx29: Set I = location of sprite for digit Vx
                    c8->I = CHIP8_FONT_ADDRESS + (c8->V[x] * 5);
                    break;
                case 0x33: // Fx33: Store BCD representation of Vx in memory at I, I+1, I+2
                    c8->memory[c8->I]     = c8->V[x] / 100;
                    c8->memory[c8->I + 1] = (c8->V[x] / 10) % 10;
                    c8->memory[c8->I + 2] = c8->V[x] % 10;
                    break;
                case 0x55: // Fx55: Store V0 through Vx in memory starting at I (Controlled by load_store_increments_i)
                    for (uint8_t reg = 0; reg <= x; reg++) {
                        c8->memory[c8->I + reg] = c8->V[reg];
                    }
                    if (c8->quirks.load_store_increments_i) {
                        c8->I += (x + 1);
                    }
                    break;
                case 0x65: // Fx65: Read V0 through Vx from memory starting at I (Controlled by load_store_increments_i)
                    for (uint8_t reg = 0; reg <= x; reg++) {
                        c8->V[reg] = c8->memory[c8->I + reg];
                    }
                    if (c8->quirks.load_store_increments_i) {
                        c8->I += (x + 1);
                    }
                    break;
            }
            break;
    }
}

void chip8_dump_registers(const Chip8 *c8) {
    if (!c8) return;
    printf("\n--- CHIP-8 CPU REGISTER DUMP [%s] ---\n", c8->quirks.preset_name);
    printf("PC: 0x%04X | I: 0x%04X | SP: 0x%02X | DT: %u | ST: %u\n",
           c8->pc, c8->I, c8->sp, c8->delay_timer, c8->sound_timer);
    for (int i = 0; i < 16; i++) {
        printf("V%X: 0x%02X  %s", i, c8->V[i], (i % 8 == 7) ? "\n" : "");
    }
    printf("----------------------------------------------------\n");
}

void chip8_render_console(const Chip8 *c8) {
    if (!c8) return;
    printf("+");
    for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) printf("-");
    printf("+\n");

    for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
        printf("|");
        for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
            uint8_t pixel = c8->display[y * CHIP8_DISPLAY_WIDTH + x];
            printf("%s", pixel ? "#" : " ");
        }
        printf("|\n");
    }

    printf("+");
    for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) printf("-");
    printf("+\n");
}
