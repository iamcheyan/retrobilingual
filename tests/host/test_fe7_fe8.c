/* Host harness for FE7/FE8: ROM gates, talk gate, message ID, A-page. */
#include "adapters/adapter.h"
#include "host_stubs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t* ewram;
static uint8_t* iwram;

static void set_proc(uint32_t base, uint32_t script) {
    uint8_t* proc = ewram + base;
    memset(proc, 0, 0x6C);
    memcpy(proc, &script, 4);
}

static int run(const RBAdapter* adapter, uint32_t active, uint32_t buffer,
               uint32_t talk, uint32_t proc_base, uint32_t script) {
    static uint8_t wram[0x40000];
    static uint8_t iwram_storage[0x8000];
    ewram = wram;
    iwram = iwram_storage;
    memset(ewram, 0, sizeof(wram));
    memset(iwram, 0, sizeof(iwram_storage));

    struct GBA gba = {0};
    gba.memory.wram = (uint32_t*)wram;
    gba.memory.iwram = (uint32_t*)iwram_storage;
    struct mCore core = {0};
    core.board = &gba;

    uint8_t* text = ewram + buffer;
    memcpy(text, "abc\3def\3", 8);
    uint16_t id = 0x1234;
    uint8_t page = 0;

    memcpy(ewram + active, &id, 2);
    set_proc(proc_base, script);
    uint32_t str = 0x02000000u + buffer + 4;
    memcpy(iwram + talk, &str, 4);
    assert(adapter->read_dialogue(&core, 0, &id, &page) == 1);
    assert(id == 0x1234 && page == 0);

    set_proc(proc_base, script);
    str = 0x02000000u + buffer + 8;
    memcpy(iwram + talk, &str, 4);
    assert(adapter->read_dialogue(&core, 0, &id, &page) == 1);
    assert(id == 0x1234 && page == 1);

    set_proc(proc_base, 0);
    assert(adapter->read_dialogue(&core, 0, &id, &page) == 0);
    return 0;
}

int main(void) {
    static const uint8_t fe7_sha[32] = {
        0x7e,0xc6,0x91,0xa7,0x41,0x18,0x64,0xab,0x51,0x76,0x50,0x28,0x15,0x6c,0x5b,0x8a,
        0x3c,0x23,0xa3,0x17,0xd2,0x3c,0x95,0xfb,0xf0,0x7a,0xc2,0x01,0x21,0x2e,0x25,0xc0,
    };
    static const uint8_t fe8_sha[32] = {
        0x44,0xfd,0x34,0x36,0x25,0xab,0x9e,0x6b,0x90,0xf6,0x3a,0x80,0x75,0x8c,0x15,0x06,
        0x6d,0x52,0x6e,0x68,0x73,0xfa,0xe9,0x14,0x74,0x00,0x63,0x14,0xa5,0xea,0xd4,0x64,
    };
    assert(rb_adapter_match(0x01000000u, fe7_sha) == &g_rb_adapter_fe7);
    assert(rb_adapter_match(0x01000000u, fe8_sha) == &g_rb_adapter_fe8);
    assert(rb_adapter_match(0x00800000u, fe7_sha) == NULL);
    assert(rb_adapter_match(0x01000000u, fe8_sha) != &g_rb_adapter_fe7);

    assert(run(&g_rb_adapter_fe7, 0x2B5B0u, 0x2A5B0u, 0x40u, 0x24E20u, 0x08BFFB84u) == 0);
    assert(run(&g_rb_adapter_fe8, 0x2B6A8u, 0x2A6A8u, 0x40u, 0x24E68u, 0x085B90F0u) == 0);
    puts("fe7/fe8 host test: ROM gates, talk gate, message ID, A-page OK");
    return 0;
}
