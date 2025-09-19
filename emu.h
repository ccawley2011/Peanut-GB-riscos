#ifndef EMU_H
#define EMU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "oslib/os.h"

typedef struct emu_state emu_state_t;

os_error *emu_create(emu_state_t **pstate, const char *rom_file_name);
void emu_update(emu_state_t *state, uint8_t *fb, size_t pitch, osbool scale);
bool emu_poll_input(emu_state_t *state);
void emu_reset(emu_state_t *state);
void emu_free(emu_state_t *state);

#endif
