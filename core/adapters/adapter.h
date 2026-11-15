#ifndef RB_ADAPTER_H
#define RB_ADAPTER_H

#include <stdint.h>
#include <stddef.h>

struct mCore;

typedef struct {
    const char* game_id;
    const char* title;
    uint32_t rom_size;
    const char* sha256_hex;
    /* Reset adapter internal state (e.g. on game reset or state load) */
    void (*reset)(void);
    /* Returns 1 if active dialogue text is found, 0 otherwise */
    int (*read_dialogue)(struct mCore* core, uint16_t* out_text_id, uint8_t* out_page);
} RBAdapter;

/* Built-in game adapters */
extern const RBAdapter g_rb_adapter_tmc;

/* Registry lookup by ROM size & SHA-256 */
const RBAdapter* rb_adapter_match(uint32_t rom_size, const uint8_t sha256[32]);

#endif /* RB_ADAPTER_H */
