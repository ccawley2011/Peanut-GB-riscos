#include "emu.h"
#include "msgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/wimp.h"

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

static os_error *check_mode(int mode) {
    os_error err = { 0, "nomode" };
    char modestr[8];

    if (os_check_mode_valid((os_mode)mode, NULL, NULL) & _C) {
        os_convert_integer1(mode, modestr, sizeof(modestr));
        return msgs_err_lookup_1(&err, modestr);
    }

    return NULL;
}

os_error *run_fullscreen(emu_state_t *state, osbool vga) {
    static const os_VDU_VAR_LIST(6) invars = {{
        os_VDUVAR_SCREEN_START,
        os_VDUVAR_DISPLAY_START,
        os_VDUVAR_LINE_LENGTH,
        os_VDUVAR_XWIND_LIMIT,
        os_VDUVAR_YWIND_LIMIT,
        os_VDUVAR_END_LIST
    }};
    int old_esc, old_fn;
    int mode = vga ? 27 : 12;
    int oldmode;
    int outvars[6];
    uint8_t *fb[2];
    os_error *err;
    size_t pitch;
    int buf = 0;
    int x, y;
    size_t i;

    err = check_mode(mode);
    if (err) {
        return err;
    }

    oldmode = osbyte2(135, 0, 255);

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

    /* Centre the image on the screen */
    x = ((outvars[3] + 1) - (LCD_WIDTH * 2)) / 2;
    if (vga)
        y = ((outvars[4] + 1) - (LCD_HEIGHT * 2)) / 2;
    else
        y = ((outvars[4] + 1) - (LCD_HEIGHT * 1)) / 2;

    fb[0] = (uint8_t *)outvars[0] + (y * outvars[2]) + (x / 2);
    fb[1] = (uint8_t *)outvars[1] + (y * outvars[2]) + (x / 2);
    pitch = outvars[2];

    old_esc = osbyte1(osbyte_VAR_ESCAPE_STATE, 0xff, 0);
    old_fn  = osbyte1(osbyte_VAR_INTERPRETATION_GROUP4, 0xc0, 0);

    while(emu_poll_input(state))
    {
        osbyte(osbyte_OUTPUT_SCREEN_BANK, buf+1, 0);
        emu_update(state, fb[buf], pitch, vga);
        osbyte(osbyte_DISPLAY_SCREEN_BANK, buf+1, 0);
        buf ^= 1;

        osbyte(osbyte_AWAIT_VSYNC, 0, 0);
    }

    osbyte(osbyte_VAR_ESCAPE_STATE, old_esc, 0);
    osbyte(osbyte_VAR_INTERPRETATION_GROUP4, old_fn, 0);

    wimp_set_mode((os_mode)oldmode);

    return NULL;
}
