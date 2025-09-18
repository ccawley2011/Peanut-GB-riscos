#include "emu.h"
#include "msgs.h"

#define PEANUT_GB_IS_LITTLE_ENDIAN 1
#define PEANUT_GB_USE_DOUBLE_WIDTH_PALETTE 1
#define PEANUT_GB_HIGH_LCD_ACCURACY 0
#define ENABLE_SOUND 0
#define ENABLE_LCD 1

/* Import emulator library. */
#include "peanut_gb.h"

#include <stdio.h>
#include <stdlib.h>

#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

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
        "erruk",
        "errop",
        "errrd",
        "errwr",
        "errhf"
    };
    emu_state_t *state = gb->direct.priv;
    os_error err = { 0, "gberr" };
    char addr[8];

    xwimp_report_error(msgs_err_lookup_2(&err, msgs_lookup(gb_err_str[gb_err], NULL),
                                         os_convert_hex4(val, addr, sizeof(addr))),
                       wimp_ERROR_BOX_OK_ICON, msgs_lookup("AppName", NULL), NULL);

    /* Free memory and then exit. */
    emu_free(state);
    exit(EXIT_FAILURE);
}

static os_error *gb_init_error(const enum gb_init_error_e gb_err)
{
    const char* gb_err_str[GB_INIT_INVALID_MAX] = {
        "errul",
        "errct",
        "errck"
    };
    os_error err = { 0, "lderr" };

    return msgs_err_lookup_1(&err, msgs_lookup(gb_err_str[gb_err], NULL));
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
static os_error *read_rom_to_ram(const char *file_name, uint8_t **prom)
{
    os_error *err;
    fileswitch_object_type type;
    int rom_size;
    uint8_t *rom = NULL;

    *prom = NULL;

    err = xosfile_read_stamped(file_name, &type, NULL, NULL, &rom_size, NULL, NULL);
    if(err != NULL)
        return err;

    if (type != fileswitch_IS_FILE)
        return xosfile_make_error(file_name, type);

    rom = malloc(rom_size);
    if (!rom) {
        return &err_nomem;
    }

    err = xosfile_load_stamped_no_path(file_name, rom, NULL, NULL, NULL, NULL, NULL);
    if(err != NULL)
    {
        free(rom);
        return err;
    }

    *prom = rom;
    return NULL;
}

os_error *emu_create(emu_state_t **pstate, const char *rom_file_name)
{
    enum gb_init_error_e ret;
    emu_state_t *state;
    os_error *err;

    *pstate = NULL;

    state = calloc(1, sizeof(emu_state_t));
    if (!state)
    {
        return &err_nomem;
    }

    /* Copy input ROM file to allocated memory. */
    err = read_rom_to_ram(rom_file_name, &state->rom);
    if(err != NULL)
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
        return gb_init_error(ret);
    }

    state->cart_ram = malloc(gb_get_save_size(&state->gb));
    if (!state->cart_ram)
    {
        emu_free(state);
        return &err_nomem;
    }

#if ENABLE_LCD
    gb_init_lcd(&state->gb, &lcd_draw_line);
#endif

    *pstate = state;
    return NULL;
}

bool emu_poll_input(emu_state_t *state)
{
    int key = osbyte1(osbyte_SCAN_KEYBOARD_LIMITED, 0, 0);
    uint8_t joypad = 0;
    bool escape = false;

    /* Scan the keyboard to see what is pressed */
    while (key < 0xff)
    {
        switch (key) {
        case 97:  /* Z */
        case 55:  /* P */
            joypad |= JOYPAD_A;
            break;
        case 66:  /* X */
        case 86:  /* L */
            joypad |= JOYPAD_B;
            break;
        case 47:  /* Backspace */
            joypad |= JOYPAD_SELECT;
            break;
        case 73:  /* Return */
            joypad |= JOYPAD_START;
            break;
        case 121: /* Right */
        case 50:  /* D */
            joypad |= JOYPAD_RIGHT;
            break;
        case 25:  /* Left */
        case 65:  /* A */
            joypad |= JOYPAD_LEFT;
            break;
        case 57:  /* Up */
        case 33:  /* W */
            joypad |= JOYPAD_UP;
            break;
        case 41:  /* Down */
        case 81:  /* S */
            joypad |= JOYPAD_DOWN;
            break;
        case 112: /* Escape */
            escape = true;
            break;
        default:
            break;
        }

        key = osbyte1(osbyte_SCAN_KEYBOARD, key + 1, 0);
    }

    state->gb.direct.joypad = ~joypad;
    return !escape;
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
    if (!state)
        return;

    free(state->cart_ram);
    free(state->rom);
    free(state);
}
