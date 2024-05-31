#ifndef EMU_H
#define EMU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum emu_err {
    EMU_OK = 0,

    EMU_NO_MEMORY,
    EMU_FILE_NOT_FOUND,
    EMU_CARTRIDGE_UNSUPPORTED,
    EMU_INVALID_CHECKSUM
} emu_err_t;

typedef enum emu_option {
    EMU_OPTION_SCALE
} emu_option_t;

typedef struct emu_state emu_state_t;

emu_err_t emu_create(emu_state_t **pstate, const char *rom_file_name);
void emu_update(emu_state_t *state, uint8_t *fb, size_t pitch);
void emu_set_option(emu_state_t *state, emu_option_t option, bool val);
void emu_reset(emu_state_t *state);
void emu_free(emu_state_t *state);

#endif
