#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CHIP8_MEMORY_SIZE    4096
#define CHIP8_NUM_REGISTERS  16
#define CHIP8_STACK_SIZE     16
#define CHIP8_KEYPAD_SIZE    16
#define CHIP8_DISPLAY_WIDTH  64
#define CHIP8_DISPLAY_HEIGHT 32
#define CHIP8_START_ADDRESS  0x200
#define CHIP8_FONT_ADDRESS   0x050

// Architectural Quirks Matrix (Hardware Era Profiles)
typedef struct {
    bool shift_reads_vy;          // 8xy6 / 8xyE: VIP reads Vy into Vx; SCHIP/Modern shifts Vx in-place
    bool load_store_increments_i; // Fx55 / Fx65: VIP increments I = I + x + 1; SCHIP leaves I unchanged
    bool logic_clears_vf;         // 8xy1 / 8xy2 / 8xy3: VIP resets VF to 0; Modern leaves VF untouched
    bool jump_offset_uses_vx;     // Bnnn / Bxnn: VIP jumps to nnn + V0; SCHIP jumps to xnn + Vx
    bool clip_sprites;            // Dxyn: true = clips at borders; false = wraps around display
    const char *preset_name;
} Chip8Quirks;

typedef enum {
    CHIP8_PRESET_COSMAC_VIP_1977, // Historical RCA COSMAC VIP 1802 fidelity
    CHIP8_PRESET_SCHIP_1990,      // HP-48 Super-CHIP 1.1 standard
    CHIP8_PRESET_MODERN_OCTO      // Modern XO-CHIP / Octo virtual machine standard
} Chip8Preset;

typedef struct {
    uint8_t     memory[CHIP8_MEMORY_SIZE];
    uint8_t     V[CHIP8_NUM_REGISTERS];
    uint16_t    I;
    uint16_t    pc;
    uint16_t    stack[CHIP8_STACK_SIZE];
    uint8_t     sp;
    uint8_t     delay_timer;
    uint8_t     sound_timer;
    uint8_t     display[CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT];
    uint8_t     keypad[CHIP8_KEYPAD_SIZE];
    bool        draw_flag;
    Chip8Quirks quirks;
} Chip8;

// Core Lifecycle APIs
void chip8_init(Chip8 *c8);
void chip8_set_quirks_preset(Chip8 *c8, Chip8Preset preset);
bool chip8_load_rom(Chip8 *c8, const uint8_t *rom_data, size_t rom_size);
void chip8_cycle(Chip8 *c8);
void chip8_update_timers(Chip8 *c8);

// Inspection & Debug APIs
uint16_t chip8_fetch_opcode(const Chip8 *c8);
void chip8_dump_registers(const Chip8 *c8);
void chip8_render_console(const Chip8 *c8);

#endif // CHIP8_H
