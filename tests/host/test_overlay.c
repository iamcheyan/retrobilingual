/* Host harness: drives the real retrobilingual.c state machine and renderer. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "host_stubs.h"

#include "retrobilingual.h"

int rb_test_pack_loaded(void);
int rb_test_state(void);
int rb_test_dialogue_page(void);
int rb_test_glyphs(void);
int rb_test_slot(void);
int rb_test_slot_has_data(int slot);
int rb_test_hud_pos(void);
int rb_test_hud_scroll(void);
int rb_test_hud_first_visible_line(void);
int rb_test_visual_filter(void);
int rb_test_autosave_interval(void);
void rb_test_autosave_start_at(int64_t now_ms);
void rb_test_autosave_tick_at(struct mCore* core, int64_t now_ms);
const char* rb_test_toast(void);
int rb_test_slot_info(int slot, char* area_out, size_t area_sz, int* hearts_out, int* rupees_out);

enum { RB_STATE_GAME = 0, RB_STATE_MENU = 1, RB_STATE_SUBTITLE = 2, RB_STATE_BINDINGS = 3, RB_STATE_CONFIRM_OVERWRITE = 4 };

static int x_down;
static int y_down;
static int16_t input_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)port; (void)device; (void)index;
    if (id == RETRO_DEVICE_ID_JOYPAD_X) return (int16_t)x_down;
    if (id == RETRO_DEVICE_ID_JOYPAD_Y) return (int16_t)y_down;
    return 0;
}
static size_t state_size(struct mCore* core) { (void)core; return 64; }
static int state_save(struct mCore* core, void* buf) { (void)core; memset(buf, 0x5A, 64); return 1; }
static int state_load(struct mCore* core, const void* buf) { (void)core; return ((const unsigned char*)buf)[0] == 0x5A; }

static uint8_t* read_file(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* data = malloc((size_t)n);
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) exit(1);
    fclose(f);
    *size = (size_t)n;
    return data;
}

int main(int argc, char** argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s <fake-rom>\n", argv[0]); return 2; }
    size_t rom_size;
    uint8_t* rom = read_file(argv[1], &rom_size);
    uint32_t* wram = calloc(0x40000, 1);
    assert(rom && wram);

    struct GBA gba = {0};
    gba.memory.rom = rom;
    gba.memory.romSize = rom_size;
    gba.memory.wram = wram;
    gba.romCrc32 = 0x6CE771A5u;

    struct mCore core = {0};
    core.board = &gba;
    core.stateSize = state_size;
    core.saveState = state_save;
    core.loadState = state_load;

    uint16_t keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_pack_loaded() == 1);

    uint8_t* ewram = (uint8_t*)wram;
    uint8_t* msg = ewram + 0x50;
    uint8_t* render = ewram + 0x22780;
    msg[0] = 0x01;
    msg[8] = 0x01;
    msg[9] = 0x02; /* text id 0x0201 */
    render[0x89] = 3;

    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_glyphs() > 0);

    /* Page advance: WAIT(3) -> ROLL(4) increments the page. */
    render[0x89] = 4;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_dialogue_page() == 1);
    assert(rb_test_glyphs() == 3); /* Exact (0201, 1), never page 0. */

    /* A single pack page serves a later game page of the same dialogue. */
    msg[8] = 0x02;
    msg[9] = 0x02; /* text id 0x0202 */
    render[0x89] = 3;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_dialogue_page() == 0);
    assert(rb_test_glyphs() == 3);
    render[0x89] = 4;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_dialogue_page() == 1);
    assert(rb_test_glyphs() == 3); /* Page 0 is the only page. */

    /* A multi-page dialogue with a missing middle page must not fall back. */
    msg[8] = 0x03;
    msg[9] = 0x02; /* text id 0x0203: pages 0 and 2 only */
    render[0x89] = 3;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_dialogue_page() == 0);
    assert(rb_test_glyphs() == 2);
    render[0x89] = 4;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_dialogue_page() == 1);
    assert(rb_test_glyphs() == 0); /* Render the FFFF missing-page notice. */

    /* Return to the exact page-1 fixture for the existing overlay checks. */
    msg[8] = 0x01;
    msg[9] = 0x02;
    render[0x89] = 3;
    retrobilingual_process_input(&core, &keys, input_cb);
    render[0x89] = 4;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_glyphs() == 3);

    /* Default chord: L (bit 9) + Select (bit 2). */
    keys = (1u << 9) | (1u << 2);
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_SUBTITLE);
    assert(retrobilingual_should_run_frame(&core) == 0);

    static uint16_t fb[240 * 160];
    memset(fb, 0x11, sizeof(fb));
    retrobilingual_draw(&core, fb, 240, 160, 240 * 2);
    int ink = 0;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 240; x++)
            if (fb[y * 240 + x] == 0xFFFF) ink++;
    assert(ink > 50);

    /* Release and press the chord again: back to the game frame. */
    keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 9) | (1u << 2);
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(retrobilingual_should_run_frame(&core) == 1);

    /* A long single pack page scrolls in the HUD without changing sDlgPage. */
    msg[8] = 0x04;
    msg[9] = 0x02; /* text id 0x0204 */
    render[0x89] = 3;
    keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    x_down = 1;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_SUBTITLE);
    x_down = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    memset(fb, 0x11, sizeof(fb));
    retrobilingual_draw(&core, fb, 240, 160, 240 * 2);
    assert(rb_test_hud_scroll() == 0);
    assert(rb_test_hud_first_visible_line() == 0);
    keys = (1u << 4); /* RIGHT */
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_scroll() == 2);
    keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 4); /* RIGHT */
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_scroll() == 4);
    assert(rb_test_dialogue_page() == 0);
    memset(fb, 0x11, sizeof(fb));
    retrobilingual_draw(&core, fb, 240, 160, 240 * 2);
    assert(rb_test_hud_first_visible_line() == 4); /* Fifth and sixth lines. */
    keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    x_down = 1;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    x_down = 0;
    retrobilingual_process_input(&core, &keys, input_cb);

    /* Y opens the menu and pauses; Y closes it. */
    y_down = 1; keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_MENU);
    assert(retrobilingual_should_run_frame(&core) == 0);
    memset(fb, 0x11, sizeof(fb));
    retrobilingual_draw(&core, fb, 240, 160, 240 * 2);
    ink = 0;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 240; x++)
            if (fb[y * 240 + x] == 0xFFFF) ink++;
    assert(ink > 20);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(retrobilingual_should_run_frame(&core) == 1);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* X button triggers subtitle overlay directly and pauses; X closes it. */
    x_down = 1; keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_SUBTITLE);
    assert(retrobilingual_should_run_frame(&core) == 0);
    memset(fb, 0x11, sizeof(fb));
    retrobilingual_draw(&core, fb, 240, 160, 240 * 2);
    ink = 0;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 240; x++)
            if (fb[y * 240 + x] == 0xFFFF) ink++;
    assert(ink > 50);
    x_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    x_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(retrobilingual_should_run_frame(&core) == 1);
    x_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Open menu with Y, change X button setting, and verify behavior. */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_MENU);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Move down 2 times to 'X BUTTON' (0: QUICK SAVE, 1: QUICK LOAD, 2: X BUTTON) */
    for (int i = 0; i < 2; i++) {
        keys = (1u << 7); /* DOWN */
        retrobilingual_process_input(&core, &keys, input_cb);
        keys = 0;
        retrobilingual_process_input(&core, &keys, input_cb);
    }
    /* Press A to cycle X button action to SAVE */
    keys = (1u << 0); /* A */
    retrobilingual_process_input(&core, &keys, input_cb);
    keys = 0;
    retrobilingual_process_input(&core, &keys, input_cb);

    /* Close menu with Y */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Press X now: should perform quick save (game remains RB_STATE_GAME, not SUBTITLE) */
    x_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    x_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Open menu with Y again and cycle twice back to SUBTITLE */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_MENU);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    for (int i = 0; i < 2; i++) {
        keys = (1u << 7); /* DOWN */
        retrobilingual_process_input(&core, &keys, input_cb);
        keys = 0;
        retrobilingual_process_input(&core, &keys, input_cb);
    }
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb);
    keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb);
    keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Press X now: back to SUBTITLE overlay */
    x_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_SUBTITLE);
    x_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    x_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    x_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Test menu save, 5-slot selection, and toast notifications */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_MENU);
    y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);

    /* Cursor is already at QUICK SAVE (item 0) by default */
    assert(rb_test_slot() == 0); /* Slot 1 */

    /* Change slot to Slot 2 using RIGHT (bit 4) */
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_slot() == 1); /* Slot 2 */
    assert(rb_test_slot_has_data(1) == 0); /* Slot 2 empty initially */

    /* Move down to QUICK LOAD (item 1) and try loading empty slot 2 */
    keys = (1u << 7); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(strstr(rb_test_toast(), "SLOT 2 IS EMPTY") != NULL);

    /* Open menu again, default is QUICK SAVE (item 0), press A directly to save to Slot 2 */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(strstr(rb_test_toast(), "SAVED TO SLOT 2") != NULL);
    assert(rb_test_slot_has_data(1) == 1); /* Slot 2 now has data! */
    assert(rb_test_slot_has_data(2) == 0); /* Slot 3 still empty */

    /* Slot 2 now has data. Trying to save again should prompt for overwrite confirmation! */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_CONFIRM_OVERWRITE); /* Confirmation prompt active! */
    /* Press B to cancel */
    keys = (1u << 1); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(strstr(rb_test_toast(), "SAVE CANCELLED") != NULL);

    /* Open menu again, trigger save, and confirm with A */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_CONFIRM_OVERWRITE);
    /* Press A to confirm overwrite */
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(strstr(rb_test_toast(), "SAVED TO SLOT 2") != NULL);

    /* Open menu again, move to QUICK LOAD (item 1) and load Slot 2 */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 7); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);
    assert(strstr(rb_test_toast(), "LOADED SLOT 2") != NULL);

    /* Open menu, move down 3 times to POSITION (item 3), test cycling AUTO -> TOP -> CENTER -> BOTTOM -> AUTO */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_pos() == 0); /* AUTO */
    for (int i = 0; i < 3; i++) {
        keys = (1u << 7); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    }
    /* Press RIGHT (bit 4) -> TOP (1) */
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_pos() == 1); /* TOP */
    /* Press RIGHT (bit 4) -> CENTER (2) */
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_pos() == 2); /* CENTER */
    /* Press RIGHT (bit 4) -> BOTTOM (3) */
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_pos() == 3); /* BOTTOM */
    /* Press A (bit 0) -> AUTO (0) */
    keys = (1u << 0); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_hud_pos() == 0); /* AUTO */
    /* Close menu */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);

    /* Verify slot info queries: Slot 1 and 2 were saved during the test; Slot 4 is empty */
    char area_buf[32];
    int hearts = 0, rupees = 0;
    assert(rb_test_slot_info(3, area_buf, sizeof(area_buf), &hearts, &rupees) == 0); /* Slot 4 empty */
    assert(rb_test_slot_info(1, area_buf, sizeof(area_buf), &hearts, &rupees) == 1); /* Slot 2 valid (host stub small size) */

    /* Create a full 0x61000 byte state for Slot 3 with mock EWRAM stats */
    const char* dir = getenv("RETROBILINGUAL_DIR");
    if (!dir) dir = "/tmp";
    char slot3_path[512];
    snprintf(slot3_path, sizeof(slot3_path), "%s/retrobilingual-slot3.state", dir);
    FILE* f3 = fopen(slot3_path, "wb");
    assert(f3 != NULL);
    uint8_t* dummy_state = (uint8_t*)calloc(1, 0x61000);
    assert(dummy_state != NULL);
    dummy_state[0x23AEB] = 24 * 8; /* 24 hearts */
    dummy_state[0x23B00] = 150 & 0xFF; /* 150 rupees */
    dummy_state[0x23B01] = (150 >> 8) & 0xFF;
    dummy_state[0x54A91] = 9; /* Location 9: NORTH HYRULE */
    assert(fwrite(dummy_state, 1, 0x61000, f3) == 0x61000);
    fclose(f3);
    free(dummy_state);

    memset(area_buf, 0, sizeof(area_buf));
    assert(rb_test_slot_info(2, area_buf, sizeof(area_buf), &hearts, &rupees) == 1);
    assert(strcmp(area_buf, "NORTH HYRULE") == 0);
    assert(hearts == 24);
    assert(rupees == 150);

    /* Open menu, switch to Slot 3 and draw */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 7); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    /* Slot is currently 1 (Slot 2). Press RIGHT to go to Slot 3 (index 2) */
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_slot() == 2);
    memset(fb, 0x11, sizeof(fb));
    retrobilingual_draw(&core, fb, 240, 160, 240 * 2);
    ink = 0;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 240; x++)
            if (fb[y * 240 + x] == 0xFFFF) ink++;
    assert(ink > 50);
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);

    /* Test Visual Filter menu navigation and presentation */
    /* Open menu with Y */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_MENU);
    /* Navigate down to FILTER (item 4: down 4 times from item 0) */
    for (int i = 0; i < 4; i++) {
        keys = (1u << 7); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    }
    int initial_filter = rb_test_visual_filter();
    /* Press RIGHT to step filter */
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_visual_filter() == (initial_filter + 1) % RB_FILTER_COUNT);
    assert(strlen(rb_test_toast()) > 0);

    /* AUTO SAVE is the sixth menu item and persists its interval. */
    keys = (1u << 7); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    keys = (1u << 4); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_autosave_interval() == 1); /* 1 minute */
    const char* autosave_dir = getenv("RETROBILINGUAL_DIR");
    assert(autosave_dir != NULL);
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/retrobilingual.conf", autosave_dir);
    FILE* config = fopen(config_path, "r");
    assert(config != NULL);
    char config_text[1024] = {0};
    size_t config_size = fread(config_text, 1, sizeof(config_text) - 1, config);
    fclose(config);
    assert(config_size > 0 && strstr(config_text, "autosave_interval=1") != NULL);
    char autosave_path[512];
    snprintf(autosave_path, sizeof(autosave_path), "%s/retrobilingual-autosave.state", autosave_dir);
    unlink(autosave_path);
    rb_test_autosave_start_at(0);
    rb_test_autosave_tick_at(&core, 0);
    rb_test_autosave_tick_at(&core, 59999);
    assert(access(autosave_path, F_OK) != 0);
    rb_test_autosave_tick_at(&core, 60000);
    assert(access(autosave_path, R_OK) == 0);
    assert(strstr(rb_test_toast(), "AUTO SAVED") != NULL);
    /* Disable again; the selected manual slot remains available. */
    keys = (1u << 5); retrobilingual_process_input(&core, &keys, input_cb); keys = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_autosave_interval() == 0);
    assert(rb_test_slot_has_data(1) == 1);

    /* Test retrobilingual_present 720x480 output */
    void* out_buf = NULL;
    unsigned out_w = 0, out_h = 0, out_pitch = 0;
    retrobilingual_present(fb, 240, 160, 240 * 2, &out_buf, &out_w, &out_h, &out_pitch);
    assert(out_buf != NULL);
    assert(out_w == 720);
    assert(out_h == 480);
    assert(out_pitch == 720 * 2);

    /* Close menu */
    y_down = 1; retrobilingual_process_input(&core, &keys, input_cb); y_down = 0; retrobilingual_process_input(&core, &keys, input_cb);
    assert(rb_test_state() == RB_STATE_GAME);

    puts("host overlay test: pack gate, page tracking, chord pause, subtitle, 5 slots, position cycling, save/load, autosave timing, visual filter OK");
    return 0;
}
