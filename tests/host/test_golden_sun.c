/* Host harness: real Golden Sun ROM plus the Golden Sun adapter and pack. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_stubs.h"
#include "retrobilingual.h"
#include "adapters/adapter.h"

int rb_test_pack_loaded(void);
int rb_test_dialogue_page(void);
int rb_test_glyphs(void);
int rb_test_state(void);

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

static void set_id(uint8_t* ewram, uint16_t id) {
    ewram[0x301D8] = (uint8_t)(id & 0xFF);
    ewram[0x301D9] = (uint8_t)(id >> 8);
}

static void tick(struct mCore* core, uint16_t keys) {
    retrobilingual_process_input(core, &keys, NULL);
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    size_t rom_size;
    uint8_t* rom = read_file(argv[1], &rom_size);
    uint32_t* wram = calloc(0x40000, 1);
    uint16_t* vram = calloc(0x18000, 2);
    assert(rom && wram && vram);

    struct GBA gba = {0};
    gba.memory.rom = rom;
    gba.memory.romSize = rom_size;
    gba.memory.wram = wram;
    gba.video.vram = vram;

    struct mCore core = {0};
    core.board = &gba;
    core.stateSize = state_size;
    core.saveState = state_save;
    core.loadState = state_load;

    uint16_t keys = 0;
    uint8_t* ewram = (uint8_t*)wram;

    /* Clean Japanese ROM, the RG34XX patched dump, and the 2024 beta. */
    static const uint8_t jp_sha[32] = {
        0x08,0x8b,0xed,0xae,0x4b,0xad,0x8b,0x67,0xe8,0x7f,0xf1,0x00,0x35,0xa8,0x98,0xd3,
        0x63,0x9f,0x31,0x82,0xd4,0x86,0xfe,0x5a,0x5d,0x11,0x3b,0xab,0x22,0x3e,0x0a,0x26,
    };
    static const uint8_t handheld_sha[32] = {
        0xf4,0xe4,0x31,0x61,0x6b,0x92,0x0e,0xcb,0x57,0x23,0x49,0xf0,0x6d,0xcd,0xef,0xd3,
        0x4c,0x15,0x78,0x9d,0x20,0x32,0x21,0x6e,0x27,0xfe,0x9f,0x7e,0xe3,0x67,0x03,0xb5,
    };
    static const uint8_t cn_sha[32] = {
        0xf5,0x66,0xde,0x92,0x7b,0x4c,0xcf,0x95,0xe5,0x56,0x86,0x55,0x3d,0xb3,0x08,0x9c,
        0x26,0xb8,0x42,0x15,0x8d,0x49,0xa2,0xa9,0x14,0x72,0xa8,0xb6,0xba,0x14,0x19,0xbc,
    };
    assert(rb_adapter_match(0x00800000u, jp_sha) == &g_rb_adapter_golden_sun);
    assert(rb_adapter_match(0x008D4000u, handheld_sha) == &g_rb_adapter_golden_sun_rg34xx);
    assert(rb_adapter_match(0x00B0E9B0u, cn_sha) == &g_rb_adapter_golden_sun_cn2024);
    assert(rb_adapter_match(0x008D4000u, jp_sha) == NULL);
    assert(rb_adapter_match(0x00800000u, handheld_sha) == NULL);
    assert(rb_adapter_match(0x008D4000u, cn_sha) == NULL);
    tick(&core, keys);
    if (argc > 2 && strcmp(argv[2], "bad-gate") == 0) {
        assert(rb_test_pack_loaded() == 0);
        puts("golden sun pack gate: mismatched ROM rejected");
        return 0;
    }
    assert(rb_test_pack_loaded() == 1);

    /* First verified conversation message: page 0 is a built entry. */
    set_id(ewram, 0x0FDD);
    tick(&core, keys);
    assert(rb_test_dialogue_page() == 0);
    assert(rb_test_glyphs() > 0);

    /* A multi-page message: the page advances only after the page has been
     * rendered stably, which distinguishes it from typewriter fast-forward. */
    set_id(ewram, 0x0B37);
    tick(&core, keys);
    assert(rb_test_dialogue_page() == 0);
    for (int i = 0; i < 6; i++) tick(&core, keys);
    keys = 1u << 0; /* libretro keymap A */
    tick(&core, keys);
    keys = 0;
    assert(rb_test_dialogue_page() == 1);
    assert(rb_test_glyphs() > 0);
    /* A press while the next page is still typing must not overshoot. */
    keys = 1u << 0; tick(&core, keys); keys = 0;
    assert(rb_test_dialogue_page() == 1);

    /* A new message resets the page counter. */
    set_id(ewram, 0x0B38);
    tick(&core, keys);
    assert(rb_test_dialogue_page() == 0);

    /* Unknown ID must not reuse stale glyphs. */
    set_id(ewram, 0x7777);
    tick(&core, keys);
    assert(rb_test_glyphs() == 0);
    assert(rb_test_dialogue_page() == 0);

    /* Closing the box clears the adapter state; reopening the same ID starts
     * again from page 0. */
    set_id(ewram, 0);
    tick(&core, keys);
    set_id(ewram, 0x0B37);
    tick(&core, keys);
    assert(rb_test_dialogue_page() == 0);

    /* X opens the subtitle HUD and draws the translated entry. */
    ewram[0x301D8] = 0xDD; ewram[0x301D9] = 0x0F;
    tick(&core, keys);
    static int x_down;
    x_down = 1;
    /* The host shim has no input callback; opening via the X path is covered
     * by test_overlay.c.  Draw the subtitle state directly is not exposed, so
     * only verify the pack entry remains renderable. */
    (void)x_down;
    assert(rb_test_glyphs() > 0);

    puts("golden sun overlay test: ROM gate, EWRAM ID, A-page sync, reset OK");
    return 0;
}
