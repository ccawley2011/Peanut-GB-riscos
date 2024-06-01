#include "emu.h"

#define PEANUT_GB_IS_LITTLE_ENDIAN 1
#define PEANUT_GB_USE_DOUBLE_WIDTH_PALETTE 1
#define PEANUT_GB_HIGH_LCD_ACCURACY 0
#define ENABLE_SOUND 0
#define ENABLE_LCD 1

/* Import emulator library. */
#include "peanut_gb.h"

#include <stdio.h>
#include <stdlib.h>

struct emu_state
{
    struct gb_s gb;

    /* Pointer to allocated memory holding GB file. */
    uint8_t *rom;
    /* Pointer to allocated memory holding save file. */
    uint8_t *cart_ram;

    /* Frame buffer */
    uint8_t *fb;
    size_t pitch;
    bool scale;
};

/**
 * Returns a byte from the ROM file at the given address.
 */
static uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
    const emu_state_t * const state = gb->direct.priv;
    return state->rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
static uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
    const emu_state_t * const state = gb->direct.priv;
    return state->cart_ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */
static void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
                              const uint8_t val)
{
    const emu_state_t * const state = gb->direct.priv;
    state->cart_ram[addr] = val;
}

static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val)
{
    const char* gb_err_str[GB_INVALID_MAX] = {
        "UNKNOWN",
        "INVALID OPCODE",
        "INVALID READ",
        "INVALID WRITE",
        "HALT FOREVER"
    };
    emu_state_t *state = gb->direct.priv;

    fprintf(stderr, "Error %d occurred: %s at %04X\n. Exiting.\n",
                    gb_err, gb_err_str[gb_err], val);

    /* Free memory and then exit. */
    emu_free(state);
    exit(EXIT_FAILURE);
}

#if ENABLE_LCD
extern void copy_160_pixels(void *dst, const void *src);
extern void copy_160_pixels_2x(void *dst, const void *src, void *dst2);

/**
 * Draws scanline into framebuffer.
 */
static void lcd_draw_line(struct gb_s *gb,
                          const uint8_t *pixels,
                          const uint_fast8_t line)
{
    emu_state_t *state = gb->direct.priv;
    uint8_t *fb = state->fb;

    if (state->scale) {
        fb += state->pitch * line * 2;
        copy_160_pixels_2x(fb, pixels, fb + state->pitch);
    } else {
        fb += state->pitch * line;
        copy_160_pixels(fb, pixels);
    }
}
#endif

/**
 * Returns a pointer to the allocated space containing the ROM. Must be freed.
 */
static emu_err_t read_rom_to_ram(const char *file_name, uint8_t **prom)
{
    FILE *rom_file;
    size_t rom_size;
    uint8_t *rom = NULL;

    *prom = NULL;

    rom_file = fopen(file_name, "rb");
    if(!rom_file)
        return EMU_FILE_NOT_FOUND;

    fseek(rom_file, 0, SEEK_END);
    rom_size = (size_t)ftell(rom_file);
    rewind(rom_file);

    rom = malloc(rom_size);
    if (!rom) {
        fclose(rom_file);
        return EMU_NO_MEMORY;
    }

    if(fread(rom, sizeof(uint8_t), rom_size, rom_file) != rom_size)
    {
        free(rom);
        fclose(rom_file);
        return EMU_FILE_NOT_FOUND;
    }

    fclose(rom_file);

    *prom = rom;
    return EMU_OK;
}

emu_err_t emu_create(emu_state_t **pstate, const char *rom_file_name)
{
    enum gb_init_error_e ret;
    emu_state_t *state;
    emu_err_t err;

    *pstate = NULL;

    state = calloc(1, sizeof(emu_state_t));
    if (!state)
    {
        return EMU_NO_MEMORY;
    }

    /* Copy input ROM file to allocated memory. */
    err = read_rom_to_ram(rom_file_name, &state->rom);
    if(err != EMU_OK)
    {
        emu_free(state);
        return err;
    }

    /* Initialise context. */
    ret = gb_init(&state->gb, &gb_rom_read, &gb_cart_ram_read,
                  &gb_cart_ram_write, &gb_error, state);

    if(ret != GB_INIT_NO_ERROR)
    {
        emu_free(state);

        switch (ret) {
        case GB_INIT_CARTRIDGE_UNSUPPORTED: return EMU_CARTRIDGE_UNSUPPORTED;
        case GB_INIT_INVALID_CHECKSUM: return EMU_INVALID_CHECKSUM;
        default: return EMU_UNKNOWN_ERROR;
        }
    }

    state->cart_ram = malloc(gb_get_save_size(&state->gb));
    if (!state->cart_ram)
    {
        emu_free(state);
        return EMU_NO_MEMORY;
    }

#if ENABLE_LCD
    gb_init_lcd(&state->gb, &lcd_draw_line);
#endif

    *pstate = state;
    return EMU_OK;
}

void emu_update(emu_state_t *state, uint8_t *fb, size_t pitch)
{
    state->fb = fb;
    state->pitch = pitch;

    /* Execute CPU cycles until the screen has to be redrawn. */
    gb_run_frame(&state->gb);
}

void emu_set_option(emu_state_t *state, emu_option_t opt, bool val)
{
    switch (opt) {
    case EMU_OPTION_SCALE:
        state->scale = val;
        break;
    }
}

void emu_reset(emu_state_t *state)
{
    gb_reset(&state->gb);
}

void emu_free(emu_state_t *state)
{
    free(state->cart_ram);
    free(state->rom);
    free(state);
}
