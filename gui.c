#include <stdlib.h>
#include <string.h>

#include "oslib/colourtrans.h"
#include "oslib/wimp.h"

#include "emu.h"
#include "msgs.h"

/* Utility functions */

static osbool quit = FALSE;
static osbool vga = TRUE;
static wimp_w menu_w;

static void translate_menu(wimp_menu *menu) {
    wimp_menu_entry *entry = menu->entries;
    const char *label;
    int label_used;

    label = msgs_lookup(menu->title_data.text, &label_used);
    if (label_used > 11)
        label_used = 11;
    memcpy(menu->title_data.text, label, label_used);
    menu->title_data.text[label_used] = 0;

    do {
        label = msgs_lookup(entry->data.text, &label_used);

        entry->data.indirected_text.text = (char *)label;
        entry->data.indirected_text.validation = NULL;
        entry->data.indirected_text.size = label_used;

        entry->icon_flags |= wimp_ICON_INDIRECTED;
    } while (!((entry++)->menu_flags & wimp_MENU_LAST));
}

static wimp_w create_window_from_template(char **data, const char *name) {
    int def_size, ws_size;
    wimp_window *window;
    char *ws;

    wimp_load_template(wimp_GET_SIZE, 0, 0, wimp_NO_FONTS,
                       (char *)name, 0, &def_size, &ws_size);

    *data = malloc(def_size + ws_size);
    window = (wimp_window *)*data;
    ws = *data + def_size;

    wimp_load_template(window, ws, ws + ws_size, wimp_NO_FONTS,
                       (char *)name, 0, NULL, NULL);

    return wimp_create_window(window);
}

void open_window(wimp_w window) {
    /* TODO: Unhardcode this! */
    wimp_open open = { window, { 0, 0, 160 << 2, 144 << 2 }, 0, 0, wimp_TOP };
    wimp_open_window(&open);
}



/* Iconbar icon code */

static wimp_MENU(3) ibar_menu = {
    { "AppName" },
    wimp_COLOUR_BLACK,
    wimp_COLOUR_LIGHT_GREY,
    wimp_COLOUR_BLACK,
    wimp_COLOUR_WHITE,
    16*8,
    wimp_MENU_ITEM_HEIGHT,
    wimp_MENU_ITEM_GAP,

    { { 0, NULL, wimp_ICON_TEXT | wimp_ICON_FILLED |
        (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
        (wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT), { "info" } },

      { 0, NULL, wimp_ICON_TEXT | wimp_ICON_FILLED |
        (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
        (wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT), { "help" } },

      { wimp_MENU_LAST, NULL, wimp_ICON_TEXT | wimp_ICON_FILLED |
        (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
        (wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT), { "quit" } } }
};

void ibar_initialise(void) {
    static const wimp_icon_create iconbar = {
        wimp_ICON_BAR_RIGHT,
        { { 0, 0, 68, 68 },
          wimp_ICON_SPRITE | (wimp_BUTTON_CLICK << wimp_ICON_BUTTON_TYPE_SHIFT),
          { "application" } }
    };
    char *data;

    wimp_create_icon(&iconbar);

    translate_menu((wimp_menu *)&ibar_menu);
    ibar_menu.entries[0].sub_menu = (wimp_menu *)create_window_from_template(&data, "ProgInfo");
}

void ibar_mouse_click(wimp_pointer *pointer)
{
    switch (pointer->buttons) {
    case wimp_CLICK_MENU:
        wimp_create_menu((wimp_menu *)&ibar_menu, pointer->pos.x - 64, 228);
        menu_w = wimp_ICON_BAR;
        break;
    }
}

void ibar_menu_click(wimp_selection *selection)
{
    switch (selection->items[0]) {
    case 1:
        xos_cli("%Filer_Run <PeanutGB$Dir>.!Help");
        break;
    case 2:
        quit = TRUE;
        break;
    }
}


/* Sprite code */

static os_PALETTE(16) default_palette = {{
    0xFFFFFF00, 0xA3A3A300, 0x52525200, 0x00000000,
    0xFFFFFF00, 0xA3A3A300, 0x52525200, 0x00000000,
    0xFFFFFF00, 0xA3A3A300, 0x52525200, 0x00000000,
    0xFFFFFF00, 0xA3A3A300, 0x52525200, 0x00000000
}};

osspriteop_area *create_sprite(int w, int h, os_mode mode) {
    const size_t area_sz = sizeof(osspriteop_area) + sizeof(osspriteop_header) + (w*h/2) + (16*8);
    osspriteop_area *area = malloc(area_sz);
    if (!area)
        return NULL;

    area->size = area_sz;
    area->sprite_count = 0;
    area->first = 16;
    area->used = 16;

    osspriteop_create_sprite(osspriteop_USER_AREA, area, "gb", TRUE, w, h, mode);

    return area;
}

void draw_sprite(osspriteop_area *area, osspriteop_id id, int x, int y,
                 const os_factors *factors, const osspriteop_trans_tab *trans_tab) {
    osspriteop_put_sprite_scaled(osspriteop_PTR, area, id, x, y,
                                 osspriteop_GIVEN_WIDE_ENTRIES, factors, trans_tab);
}

void update_sprite(osspriteop_area *area, osspriteop_id id, const os_factors *factors,
                   const osspriteop_trans_tab *trans_tab, wimp_w window) {
    /* TODO: Unhardcode this! */
    wimp_draw draw;
    draw.w      = window;
    draw.box.x0 = 0;
    draw.box.y0 = 0;
    draw.box.x1 = 160 << 2;
    draw.box.y1 = 144 << 2;

    osbool more = wimp_update_window(&draw);
    while (more) {
        draw_sprite(area, id, draw.box.x0, draw.box.y0, factors, trans_tab);
        more = wimp_get_rectangle(&draw);
    }
}



/* Instance management */

typedef struct instance {
    struct instance *next;

    emu_state_t *state;

    wimp_w w;
    char *data;

    osspriteop_area *area;
    osspriteop_id id;

    int width, height;
    os_factors factors;
    osspriteop_trans_tab *trans_tab;

    unsigned char *pixels;
    size_t pitch;
} instance_t;
static instance_t *instance_base = NULL;
static instance_t *instance_focus = NULL;


bool update_trans_tab(instance_t *instance) {
    colourtrans_table_flags flags = colourtrans_GIVEN_SPRITE;
    osspriteop_trans_tab *trans_tab;
    int log2bpp, size;

    os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_LOG2_BPP, &log2bpp);
    if (log2bpp > 3)
        flags |= colourtrans_RETURN_WIDE_ENTRIES;

    size = colourtrans_generate_table_for_sprite(instance->area, instance->id,
      os_CURRENT_MODE, colourtrans_CURRENT_PALETTE,
      NULL, flags, NULL, NULL);

    trans_tab = (osspriteop_trans_tab *)malloc(size);
    if (!trans_tab)
        return false;

    colourtrans_generate_table_for_sprite(instance->area, instance->id,
      os_CURRENT_MODE, colourtrans_CURRENT_PALETTE,
      trans_tab, flags, NULL, NULL);

    free(instance->trans_tab);
    instance->trans_tab = trans_tab;
    return true;
}


bool update_factors(instance_t *instance, os_box *visible) {
    int width  = visible ? visible->x1 - visible->x0 : instance->width;
    int height = visible ? visible->y1 - visible->y0 : instance->height;
    int xeig = 1, yeig = 1;

    os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_XEIG_FACTOR, &xeig);
    os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_YEIG_FACTOR, &yeig);

    instance->factors.xmul = width>>xeig;
    instance->factors.ymul = height>>yeig;
    instance->factors.xdiv = 160<<1;
    instance->factors.ydiv = 144;
    instance->width  = width;
    instance->height = height;
    return true;
}


static wimp_MENU(2) instance_menu = {
    { "AppName" },
    wimp_COLOUR_BLACK,
    wimp_COLOUR_LIGHT_GREY,
    wimp_COLOUR_BLACK,
    wimp_COLOUR_WHITE,
    16*12,
    wimp_MENU_ITEM_HEIGHT,
    wimp_MENU_ITEM_GAP,

    { { 0, NULL, wimp_ICON_TEXT | wimp_ICON_FILLED |
        (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
        (wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT), { "fullscreen" } },

      { wimp_MENU_TICKED | wimp_MENU_LAST, NULL, wimp_ICON_TEXT | wimp_ICON_FILLED |
        (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
        (wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT), { "vga" } } }
};


void instance_initialise(void) {
    translate_menu((wimp_menu *)&instance_menu);
}


void free_instance(instance_t *instance) {
    if (!instance)
        return;

    if (menu_w == instance->w)
        menu_w = 0;

    if (instance_focus == instance)
        instance_focus = NULL;

    if (instance_base != instance) {
        instance_t *next = instance_base;
        while (next && next->next != instance)
            next = next->next;
        if (next)
            next->next = instance->next;
    } else {
        instance_base = instance->next;
    }

    if (instance->w)
        wimp_delete_window(instance->w);
    emu_free(instance->state);
    free(instance->trans_tab);
    free(instance->area);
    free(instance->data);
    free(instance);
}


bool new_instance(const char *rom_file_name) {
    instance_t *instance = NULL;
    osspriteop_header *sprite;
    os_error *err;

    instance = calloc(1, sizeof(instance_t));
    if (!instance) {
        err = &err_nomem;
        goto cleanup;
    }

    err = emu_create(&instance->state, rom_file_name);
    if (err != NULL)
        goto cleanup;

    /* TODO: Use mode 9 in the desktop? */
    instance->area = create_sprite(160*2, 144, os_MODE4BPP90X45);
    if (!instance->area) {
        err = &err_nomem;
        goto cleanup;
    }
    sprite = (osspriteop_header *)(instance->area + 1);
    instance->id = (osspriteop_id)(sprite);
    instance->pixels = (unsigned char *)(sprite + 1) + (16*8);
    instance->pitch = 160;
    instance->width = 160<<2;
    instance->height = 144<<2;

    colourtrans_write_palette(instance->area, instance->id,
                              (os_palette const *)&default_palette,
                              colourtrans_PALETTE_FOR_SPRITE);
    update_trans_tab(instance);
    update_factors(instance, NULL);

    instance->w = create_window_from_template(&instance->data, "main");
    open_window(instance->w);

    wimp_set_caret_position(instance->w, wimp_ICON_WINDOW,
                            0, 0, 1 << 25, 0);

    instance->next = instance_base;
    instance_base = instance;

    return true;

cleanup:
    if (err) {
        xwimp_report_error(err, wimp_ERROR_BOX_OK_ICON, msgs_lookup("AppName", NULL), NULL);
    }
    free_instance(instance);
    return false;
}


instance_t *get_instance(wimp_w w) {
    instance_t *instance = instance_base;
    while (instance) {
        if (w == instance->w)
            return instance;
        instance = instance->next;
    }
    return NULL;
}


void instance_mouse_click(instance_t *instance, wimp_pointer *pointer)
{
    switch (pointer->buttons) {
    case wimp_CLICK_MENU:
        wimp_create_menu((wimp_menu *)&instance_menu, pointer->pos.x - 64, pointer->pos.y);
        menu_w = instance->w;
        break;
    default:
        wimp_set_caret_position(instance->w, wimp_ICON_WINDOW,
                                0, 0, 1 << 25, 0);
        break;
    }
}


os_error *run_fullscreen(emu_state_t *state, osbool vga);

void instance_menu_click(instance_t *instance, wimp_selection *selection)
{
    os_error *err;

    switch (selection->items[0]) {
    case 0:
        err = run_fullscreen(instance->state, vga);
        if (err != NULL) {
            xwimp_report_error(err, wimp_ERROR_BOX_OK_ICON, msgs_lookup("AppName", NULL), NULL);
        }
        break;
    case 1:
        instance_menu.entries[1].menu_flags ^= wimp_MENU_TICKED;
        vga = !vga;
        break;
    }
}


/* Main loop */

void gui_run(void)
{
    wimp_block block;
    wimp_event_no reason;
    os_t next_update = os_read_monotonic_time() + (100/60);

    while (!quit) {
        os_t t = os_read_monotonic_time();
        if (t < next_update)
            reason = wimp_poll_idle (0, &block, next_update, NULL);
        else
            reason = wimp_poll(0, &block, NULL);

        switch (reason) {
        case wimp_NULL_REASON_CODE: {
            /* TODO: Implement timing correctly! */
            instance_t *instance = instance_base;
            next_update = os_read_monotonic_time() + (100/60);
            while (instance) {
                if (instance == instance_focus)
                    emu_poll_input(instance->state);
                emu_update(instance->state, instance->pixels, instance->pitch, false);
                update_sprite(instance->area, instance->id, &instance->factors, instance->trans_tab, instance->w);
                instance = instance->next;
            }
            }
            break;

        case wimp_REDRAW_WINDOW_REQUEST: {
            osbool more = wimp_redraw_window (&(block.redraw));
            while (more) {
                instance_t *instance = get_instance(block.redraw.w);
                if (instance) {
                    draw_sprite(instance->area, instance->id, block.redraw.box.x0, block.redraw.box.y0, &instance->factors, instance->trans_tab);
                }
                more = wimp_get_rectangle (&(block.redraw));
            }
            }
            break;

        case wimp_OPEN_WINDOW_REQUEST: {
            instance_t *instance = get_instance(block.open.w);
            if (instance) {
                update_factors(instance, &(block.open.visible));
            }
            wimp_open_window(&(block.open));
            }
            break;

        case wimp_CLOSE_WINDOW_REQUEST: {
            instance_t *instance = get_instance(block.close.w);
            if (instance) {
                free_instance(instance);
            } else {
                wimp_close_window(block.close.w);
            }
            }
            break;

        case wimp_MOUSE_CLICK:
            if (block.pointer.w == wimp_ICON_BAR) {
                ibar_mouse_click(&(block.pointer));
            } else {
                instance_t *instance = get_instance(block.pointer.w);
                if (instance) {
                    instance_mouse_click(instance, &(block.pointer));
                }
            }
            break;

        case wimp_MENU_SELECTION:
            if (menu_w == wimp_ICON_BAR) {
                ibar_menu_click(&(block.selection));
            } else {
                instance_t *instance = get_instance(menu_w);
                if (instance) {
                    instance_menu_click(instance, &(block.selection));
                }
            }
            break;

        case wimp_LOSE_CARET:
            if (instance_focus && instance_focus->w == block.caret.w)
                instance_focus = NULL;
            break;

        case wimp_GAIN_CARET: {
            instance_t *instance = get_instance(block.caret.w);
            if (instance) {
                instance_focus = instance;
            }
            }
            break;

        case wimp_USER_MESSAGE:
        case wimp_USER_MESSAGE_RECORDED:
            switch (block.message.action) {
            case message_QUIT:
                quit = TRUE;
                break;
            case message_PALETTE_CHANGE:
            case message_MODE_CHANGE: {
                    instance_t *instance = instance_base;
                    while (instance) {
                        update_trans_tab(instance);
                        update_factors(instance, NULL);
                        instance = instance->next;
                    }
                }
                break;
            case message_DATA_OPEN:
                if (block.message.data.data_xfer.file_type != 0x0D7)
                    break;
                /* fall through */
            case message_DATA_LOAD:
                block.message.your_ref = block.message.my_ref;
                block.message.action = message_DATA_LOAD_ACK;
                new_instance(block.message.data.data_xfer.file_name);
                wimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE,
                                  &(block.message), wimp_BROADCAST);
                break;
            }
            break;
        }
    }
}


int main(int argc, char **argv)
{
    os_error *err;
    int i;

    instance_base = NULL;
    quit = FALSE;

    err = msgs_open("<PeanutGB$Dir>.Messages");
    if (err != NULL) {
        xwimp_report_error(err, wimp_ERROR_BOX_OK_ICON, "Peanut GB", NULL);
        return EXIT_FAILURE;
    }

    wimp_initialise(wimp_VERSION_RO2, msgs_lookup("AppName", NULL), NULL, NULL);

    wimp_open_template("<PeanutGB$Dir>.Templates");

    ibar_initialise();
    instance_initialise();

    for (i = 1; i < argc; i++)
        new_instance(argv[i]);

    gui_run();

    wimp_close_template();
    wimp_close_down(0);

    msgs_close();

    return EXIT_SUCCESS;
}
