/* Host harness for the FE6 adapter: exact ROM gate, message ID, talk gate,
 * and A-press page tracking. */
#include "adapters/adapter.h"
#include "host_stubs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FE6_SIZE 0x00800000u
#define ACTIVE 0x2A404u
#define BUF 0x29404u
#define TALK 0x38u
#define PROC 0x23CC4u
#define PROC_SIZE 0x6Cu

static uint8_t* ewram;
static uint8_t* iwram;

static void set_talk_proc(int present) {
    memset(ewram + PROC, 0, PROC_SIZE);
    if (present) {
        uint32_t script = 0x085C3D04u;
        memcpy(ewram + PROC, &script, 4);
    }
}

int main(void) {
    static const uint8_t fe6_sha[32] = {
        0xe6,0x22,0x88,0x88,0x35,0x44,0x70,0x5b,0x18,0xf1,0xa0,0x75,0x38,0x96,0xfd,0xd8,
        0x65,0xa6,0x28,0xfb,0x45,0x89,0x13,0x58,0x13,0xb1,0x6a,0x97,0x2a,0x4c,0x15,0x57,
    };
    assert(rb_adapter_match(FE6_SIZE, fe6_sha) == &g_rb_adapter_fe6);
    assert(rb_adapter_match(16 * 1024 * 1024, fe6_sha) == NULL);
    assert(rb_adapter_match(FE6_SIZE, NULL) == NULL);

    static uint8_t wram[0x40000];
    static uint8_t iwram_storage[0x8000];
    ewram = wram;
    iwram = iwram_storage;

    struct GBA gba = {0};
    gba.memory.wram = (uint32_t*)wram;
    gba.memory.iwram = (uint32_t*)iwram_storage;
    struct mCore core = {0};
    core.board = &gba;

    g_rb_adapter_fe6.reset();
    uint16_t id = 0;
    uint8_t page = 0;
    memset(ewram, 0, 0x40000);
    memcpy(ewram + BUF, "abc\3def\3", 8);
    *(uint32_t*)(ewram + ACTIVE) = 0x1234;
    set_talk_proc(1);

    /* First page completed and waiting: one 0x03 consumed, page 0. */
    *(uint32_t*)(iwram + TALK) = 0x02000000u + BUF + 4;
    assert(g_rb_adapter_fe6.read_dialogue(&core, 0, &id, &page) == 1);
    assert(id == 0x1234);
    assert(page == 0);

    /* Second page completed and waiting: two 0x03 consumed, page 1. */
    *(uint32_t*)(iwram + TALK) = 0x02000000u + BUF + 8;
    assert(g_rb_adapter_fe6.read_dialogue(&core, 0, &id, &page) == 1);
    assert(id == 0x1234);
    assert(page == 1);

    /* Still typing the first page: no wait consumed yet. */
    *(uint32_t*)(iwram + TALK) = 0x02000000u + BUF + 2;
    assert(g_rb_adapter_fe6.read_dialogue(&core, 0, &id, &page) == 1);
    assert(page == 0);

    /* Talk proc gone: the cached ID must not keep the HUD open. */
    set_talk_proc(0);
    assert(g_rb_adapter_fe6.read_dialogue(&core, 0, &id, &page) == 0);

    puts("fe6 host test: ROM gate, active message, talk gate, A-page OK");
    return 0;
}
