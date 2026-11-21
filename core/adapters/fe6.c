#include "adapter.h"

#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>

/* Fire Emblem: The Binding Blade, Japanese AFEJ01 rev0. The ROM is 8 MiB;
 * sha256sum of the clean dump in PJOS-LINUX/resouces is recorded below. */
#define FE6_JA_ROM_SIZE 0x00800000u
#define FE6_JA_SHA256 "e62288883544705b18f1a0753896fdd865a628fb4589135813b16a972a4c1557"

/* Exact EWRAM/IWRAM locations from the matching FE6 decompilation symbols.
 * sActiveMsg follows the 0x1000-byte sMsgString buffer in EWRAM. */
#define FE6_ACTIVE_MSG_OFFSET 0x0002A404u /* EWRAM 0x0202A404 */
#define FE6_TALK_STATE_OFFSET 0x00000038u /* IWRAM 0x03000038 */
#define FE6_MSG_BUFFER_OFFSET 0x00029404u /* EWRAM 0x02029404 */
#define FE6_MSG_BUFFER_SIZE   0x00001000u
#define FE6_TALK_STR_OFFSET   0x00u
#define FE6_TALK_LINE_ACTIVE  0x09u
#define FE6_PAGE_WAIT_CODE    0x03u

static void fe6_reset(void) {
}

static int fe6_read_dialogue(struct mCore* core, uint16_t keys,
                             uint16_t* out_text_id, uint8_t* out_page) {
    (void)keys;
    if (!core || !core->board || !out_text_id || !out_page) return 0;

    const struct GBA* gba = (const struct GBA*)core->board;
    if (!gba->memory.wram || !gba->memory.iwram) return 0;
    const uint8_t* ewram = (const uint8_t*)gba->memory.wram;
    const uint8_t* iwram = (const uint8_t*)gba->memory.iwram;

    /* The talk proc holds a pointer into the decoded message buffer while a
     * dialogue is active. This gates the persistent message cache against
     * stale IDs left behind after a box closes. */
    const uint8_t* talk = iwram + FE6_TALK_STATE_OFFSET;
    uint32_t str_addr = (uint32_t)talk[FE6_TALK_STR_OFFSET]
        | ((uint32_t)talk[FE6_TALK_STR_OFFSET + 1] << 8)
        | ((uint32_t)talk[FE6_TALK_STR_OFFSET + 2] << 16)
        | ((uint32_t)talk[FE6_TALK_STR_OFFSET + 3] << 24);
    uint32_t buf_addr = 0x02000000u + FE6_MSG_BUFFER_OFFSET;
    if (str_addr < buf_addr || str_addr >= buf_addr + FE6_MSG_BUFFER_SIZE)
        return 0;

    uint32_t text_id = (uint32_t)ewram[FE6_ACTIVE_MSG_OFFSET]
        | ((uint32_t)ewram[FE6_ACTIVE_MSG_OFFSET + 1] << 8)
        | ((uint32_t)ewram[FE6_ACTIVE_MSG_OFFSET + 2] << 16)
        | ((uint32_t)ewram[FE6_ACTIVE_MSG_OFFSET + 3] << 24);
    if (text_id > 0xFFFFu) return 0;

    /* FE6 talk.c increments str past control 0x03 before waiting for A/B or
     * the D-pad. Count consumed wait controls, excluding the one immediately
     * behind the cursor while that page is still waiting. */
    const uint8_t* text = ewram + FE6_MSG_BUFFER_OFFSET;
    uint32_t cursor = str_addr - buf_addr;
    unsigned waits = 0;
    for (uint32_t i = 0; i < cursor; ++i) {
        if (text[i] == FE6_PAGE_WAIT_CODE) waits++;
    }
    int waiting = cursor > 0 && text[cursor - 1] == FE6_PAGE_WAIT_CODE;
    unsigned page = waits - (waiting && waits > 0 ? 1u : 0u);
    if (page > 0xFFu) page = 0xFFu;

    *out_text_id = (uint16_t)text_id;
    *out_page = (uint8_t)page;
    return 1;
}

const RBAdapter g_rb_adapter_fe6 = {
    .game_id = "fe6_jp",
    .title = "Fire Emblem: The Binding Blade (Japan, AFEJ01 rev0)",
    .rom_size = FE6_JA_ROM_SIZE,
    .sha256_hex = FE6_JA_SHA256,
    .reset = fe6_reset,
    .read_dialogue = fe6_read_dialogue,
    .page_slack = 0,
};
