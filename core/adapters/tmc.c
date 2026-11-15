#include "adapter.h"
#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>
#include <string.h>

/* The Legend of Zelda: The Minish Cap (Japan) */
#define TMC_JP_ROM_SIZE 0x01000000
#define TMC_JP_SHA256   "16ac2572ba17e9cb2a70093d41f97ef8cff66c56417e45ea98adacdc51bb4b38"

static uint16_t sTmcTextId;
static uint8_t  sTmcPage;
static uint8_t  sTmcRender;

static void tmc_reset(void) {
    sTmcTextId = 0;
    sTmcPage = 0;
    sTmcRender = 0;
}

static int tmc_read(struct mCore* core, uint16_t* out_text_id, uint8_t* out_page) {
    if (!core) return 0;
    const struct GBA* gba = (const struct GBA*)core->board;
    if (!gba || !gba->memory.ewram) return 0;

    /*
     * EWRAM layout for TMC dialogue engine:
     * 0x02001404: 16-bit textIndex (dialogue ID)
     * 0x02001489: 8-bit text rendering state machine:
     *             3 = WAIT (typewriter text finished, waiting for player A button)
     *             4 = ROLL/NEXT (player pressed A to flip to next page)
     */
    uint32_t msg_base = 0x02001404;
    uint16_t textId = *(const uint16_t*)(gba->memory.ewram + (msg_base - 0x02000000));
    uint8_t render = *(const uint8_t*)(gba->memory.ewram + (0x02001489 - 0x02000000));

    if (textId == 0 || textId == 0xFFFF) {
        return 0;
    }

    if (textId != sTmcTextId) {
        sTmcTextId = textId;
        sTmcPage = 0;
    } else if (sTmcRender == 3 && render == 4) {
        if (sTmcPage < 0xFF) sTmcPage++;
    }
    sTmcRender = render;

    *out_text_id = sTmcTextId;
    *out_page = sTmcPage;
    return 1;
}

const RBAdapter g_rb_adapter_tmc = {
    .game_id = "zelda_minish_cap",
    .title = "The Legend of Zelda: The Minish Cap (Japan)",
    .rom_size = TMC_JP_ROM_SIZE,
    .sha256_hex = TMC_JP_SHA256,
    .reset = tmc_reset,
    .read_dialogue = tmc_read,
};
