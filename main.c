#include "emu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "oslib/os.h"
#include "oslib/osbyte.h"

#define LCD_WIDTH 160
#define LCD_HEIGHT 144

static const unsigned char vdu_setup[] = {
    /* Set default OBJ0 palette */
    os_VDU_SET_PALETTE, 0x00, 0x10, 0xFF, 0xFF, 0xFF,
    os_VDU_SET_PALETTE, 0x01, 0x10, 0xA3, 0xA3, 0xA3,
    os_VDU_SET_PALETTE, 0x02, 0x10, 0x52, 0x52, 0x52,
    os_VDU_SET_PALETTE, 0x03, 0x10, 0x00, 0x00, 0x00,
    /* Set default OBJ1 palette */
    os_VDU_SET_PALETTE, 0x04, 0x10, 0xFF, 0xFF, 0xFF,
    os_VDU_SET_PALETTE, 0x05, 0x10, 0xA3, 0xA3, 0xA3,
    os_VDU_SET_PALETTE, 0x06, 0x10, 0x52, 0x52, 0x52,
    os_VDU_SET_PALETTE, 0x07, 0x10, 0x00, 0x00, 0x00,
    /* Set default BG palette */
    os_VDU_SET_PALETTE, 0x08, 0x10, 0xFF, 0xFF, 0xFF,
    os_VDU_SET_PALETTE, 0x09, 0x10, 0xA3, 0xA3, 0xA3,
    os_VDU_SET_PALETTE, 0x0A, 0x10, 0x52, 0x52, 0x52,
    os_VDU_SET_PALETTE, 0x0B, 0x10, 0x00, 0x00, 0x00,
    /* Set extra palette */
    os_VDU_SET_PALETTE, 0x0C, 0x10, 0xFF, 0xFF, 0xFF,
    os_VDU_SET_PALETTE, 0x0D, 0x10, 0xA3, 0xA3, 0xA3,
    os_VDU_SET_PALETTE, 0x0E, 0x10, 0x52, 0x52, 0x52,
    os_VDU_SET_PALETTE, 0x0F, 0x10, 0x00, 0x00, 0x00,
    /* Set the border colour */
    os_VDU_SET_PALETTE, 0x00, 0x18, 0x00, 0x00, 0x00,

    /* Disable the text cursor */
    os_VDU_MISC, os_MISC_CURSOR, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    /* Set the text colours */
    os_VDU_SET_TEXT_COLOUR, 0xC, 17, 0x8F,

    /* Set the background colour */
    os_VDU_SET_GCOL, 0, 0x8F
};

static void setup_fullscreen(uint8_t **fb, size_t *pitch, int mode, int yscale) {
    static const os_VDU_VAR_LIST(6) invars = {{
        os_VDUVAR_SCREEN_START,
        os_VDUVAR_DISPLAY_START,
        os_VDUVAR_LINE_LENGTH,
        os_VDUVAR_XWIND_LIMIT,
        os_VDUVAR_YWIND_LIMIT,
        os_VDUVAR_END_LIST
    }};
    int outvars[6];
    int x, y;
    size_t i;

    /* Switch to the requested mode */
    os_writec(os_VDU_SET_MODE);
    os_writec(mode);

    /* Perform initial scren setup */
    for (i = 0; i < sizeof(vdu_setup); i++)
        os_writec(vdu_setup[i]);

    /* Clear both display banks */
    osbyte(osbyte_OUTPUT_SCREEN_BANK, 2, 0);
    os_writec(os_VDU_CLG);
    osbyte(osbyte_OUTPUT_SCREEN_BANK, 1, 0);
    os_writec(os_VDU_CLG);
    osbyte(osbyte_DISPLAY_SCREEN_BANK, 2, 0);

    os_read_vdu_variables((os_vdu_var_list const *)&invars, outvars);

    x = ((outvars[3] + 1) - (LCD_WIDTH * 2)) / 2;
    y = ((outvars[4] + 1) - (LCD_HEIGHT * yscale)) / 2;

    fb[0] = (uint8_t *)outvars[0] + (y * outvars[2]) + (x / 2);
    fb[1] = (uint8_t *)outvars[1] + (y * outvars[2]) + (x / 2);
    *pitch = outvars[2];
}

int gbmain(int argc, char **argv)
{
    const char *rom_file_name = NULL;
    emu_state_t *state;
    os_error *err;
    int i;

    bool benchmark = false;
    bool scale = true;

    uint8_t *fb[2];
    size_t pitch;
    int buf = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0)
            benchmark = true;
        else if (strcmp(argv[i], "--scale") == 0)
            scale = true;
        else
            rom_file_name = argv[i];
    }

    if (!rom_file_name) {
        printf("Syntax: %s [--benchmark] [--scale] <ROM>\n", argv[0]);
        exit(EXIT_FAILURE);
    } 

    err = emu_create(&state, rom_file_name);

    if (err != NULL)
    {
        printf("Peanut-GB failed to initialise: %s\n", err->errmess);
        exit(EXIT_FAILURE);
    }

    if (scale)
        setup_fullscreen(fb, &pitch, 27, 2);
    else
        setup_fullscreen(fb, &pitch, 12, 1);
    emu_set_option(state, EMU_OPTION_SCALE, scale);

    if (benchmark)
    {
        for (i = 0; i < 5; i++)
        {
            /* Start benchmark. */
            const uint_fast32_t frames_per_run = 1 * 1024;
            clock_t start_time, end_time;
            uint_fast32_t frames = 0;

            osbyte(osbyte_DISPLAY_SCREEN_BANK, 1, 0);
            printf("Run %u: ", i);
#ifndef __archie__
            fflush(stdout);
#endif

            osbyte(osbyte_OUTPUT_SCREEN_BANK, 2, 0);
            start_time = clock();

            do
            {
                /* Execute CPU cycles until the screen has to be
                 * redrawn. */
                emu_update(state, fb[1], pitch);
            }
            while(++frames < frames_per_run);

            end_time = clock();
            osbyte(osbyte_OUTPUT_SCREEN_BANK, 1, 0);

            {
                double duration =
                        (double)(end_time - start_time) / CLOCKS_PER_SEC;
                double fps = frames / duration;
                printf("%f FPS, dur: %f\n", fps, duration);
            }

            emu_reset(state);
        }
    } else {
        osbyte(osbyte_VAR_ESCAPE_STATE, 1, 0);

        while(emu_poll_input(state))
        {
            osbyte(osbyte_OUTPUT_SCREEN_BANK, buf+1, 0);
            emu_update(state, fb[buf], pitch);
            osbyte(osbyte_DISPLAY_SCREEN_BANK, buf+1, 0);
            buf ^= 1;

            osbyte(osbyte_AWAIT_VSYNC, 0, 0);
        }

        osbyte(osbyte_VAR_ESCAPE_STATE, 0, 0);
    }

    emu_free(state);
    return EXIT_SUCCESS;
}
