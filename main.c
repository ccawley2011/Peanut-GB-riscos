#include "emu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kernel.h"
#include "swis.h"

#define LCD_WIDTH 160
#define LCD_HEIGHT 144

static const unsigned char vdu_setup[] = {
    /* Set default OBJ0 palette */
    0x13, 0x00, 0x10, 0xFF, 0xFF, 0xFF,
    0x13, 0x01, 0x10, 0xA3, 0xA3, 0xA3,
    0x13, 0x02, 0x10, 0x52, 0x52, 0x52,
    0x13, 0x03, 0x10, 0x00, 0x00, 0x00,
    /* Set default OBJ1 palette */
    0x13, 0x04, 0x10, 0xFF, 0xFF, 0xFF,
    0x13, 0x05, 0x10, 0xA3, 0xA3, 0xA3,
    0x13, 0x06, 0x10, 0x52, 0x52, 0x52,
    0x13, 0x07, 0x10, 0x00, 0x00, 0x00,
    /* Set default BG palette */
    0x13, 0x08, 0x10, 0xFF, 0xFF, 0xFF,
    0x13, 0x09, 0x10, 0xA3, 0xA3, 0xA3,
    0x13, 0x0A, 0x10, 0x52, 0x52, 0x52,
    0x13, 0x0B, 0x10, 0x00, 0x00, 0x00,
    /* Set extra palette */
    0x13, 0x0C, 0x10, 0xFF, 0xFF, 0xFF,
    0x13, 0x0D, 0x10, 0xA3, 0xA3, 0xA3,
    0x13, 0x0E, 0x10, 0x52, 0x52, 0x52,
    0x13, 0x0F, 0x10, 0x00, 0x00, 0x00,
    /* Set the border colour */
    0x13, 0x00, 0x18, 0x00, 0x00, 0x00,

    /* Disable the text cursor */
    23, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    /* Set the text colours */
    17, 0xC, 17, 0x8F,

    /* Set the background colour */
    18, 0, 0x8F
};

static void setup_fullscreen(uint8_t **fb, size_t *pitch, int mode, int yscale) {
    static const int invars[6] = { 148, 149, 6, 11, 12, -1 };
    int outvars[6];
    _kernel_swi_regs regs;
    int x, y;
    size_t i;

    /* Switch to the requested mode */
    _kernel_oswrch(22);
    _kernel_oswrch(mode);

    /* Perform initial scren setup */
    for (i = 0; i < sizeof(vdu_setup); i++)
        _kernel_oswrch(vdu_setup[i]);

    /* Clear both display banks */
    _kernel_osbyte(112, 2, 0);
    _kernel_oswrch(16);
    _kernel_osbyte(112, 1, 0);
    _kernel_oswrch(16);
    _kernel_osbyte(113, 2, 0);

    regs.r[0] = (int)invars;
    regs.r[1] = (int)outvars;
    _kernel_swi(OS_ReadVduVariables, &regs, &regs);

    x = ((outvars[3] + 1) - (LCD_WIDTH * 2)) / 2;
    y = ((outvars[4] + 1) - (LCD_HEIGHT * yscale)) / 2;

    fb[0] = (uint8_t *)outvars[0] + (y * outvars[2]) + (x / 2);
    fb[1] = (uint8_t *)outvars[1] + (y * outvars[2]) + (x / 2);
    *pitch = outvars[2];
}

int main(int argc, char **argv)
{
    const char *rom_file_name = NULL;
    emu_state_t *state;
    emu_err_t err;
    int i;

    bool benchmark = false;
    bool scale = false;

    uint8_t *fb[2];
    size_t pitch;
    int buf = 0;

    for (i = 1; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
        if (strcmp(argv[i], "--benchmark") == 0)
            benchmark = true;
        else if (strcmp(argv[i], "--scale") == 0)
            scale = true;
        else
            rom_file_name = argv[i];
    }

    if (!rom_file_name) {
        fprintf(stderr, "Syntax: %s [--benchmark] [--scale] <ROM>\n", argv[0]);
        exit(EXIT_FAILURE);
    } 

    err = emu_create(&state, rom_file_name);

    if (err != EMU_OK)
    {
        fprintf(stderr, "Peanut-GB failed to initialise: %d\n",
                        err);
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

            _kernel_osbyte(113, 1, 0);
            printf("Run %u: ", i);
            fflush(stdout);

            _kernel_osbyte(112, 2, 0);
            start_time = clock();

            do
            {
                /* Execute CPU cycles until the screen has to be
                 * redrawn. */
                emu_update(state, fb[1], pitch);
            }
            while(++frames < frames_per_run);

            end_time = clock();
            _kernel_osbyte(112, 1, 0);

            {
                double duration =
                        (double)(end_time - start_time) / CLOCKS_PER_SEC;
                double fps = frames / duration;
                printf("%f FPS, dur: %f\n", fps, duration);
            }

            emu_reset(state);
        }
    } else {
        while(1)
        {
            _kernel_osbyte(112, buf+1, 0);
            emu_update(state, fb[buf], pitch);
            _kernel_osbyte(113, buf+1, 0);
            buf ^= 1;

            //_kernel_osbyte(19, 0, 0);
        }
    }

    emu_free(state);
    return EXIT_SUCCESS;
}
