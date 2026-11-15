# Handheld Hardware Key Mapping & DTS Debugging Guide

When deploying RetroBilingual to Linux-based retro handhelds (Anbernic RG34XX, Powkiddy, Miyoo, etc.), developers often encounter button inversion issues (e.g. physical X triggering Y or vice-versa). This document explains the exact root cause and debugging methodology.

---

## 1. The "Xbox vs. Nintendo" Layout Trap

On Nintendo layout handhelds (e.g. RG34XX), the diamond face buttons are arranged:
```
       [ X ]  <-- Top button
 [ Y ]       [ A ]
       [ B ]
```

Under Linux `<linux/input-event-codes.h>`:
- `BTN_NORTH` = `307` (North / Top position)
- `BTN_WEST`  = `306` (West / Left position)
- `BTN_EAST`  = `305` (East / Right position)
- `BTN_SOUTH` = `304` (South / Bottom position)

### The Vendor DTS Pitfall
SoC vendor firmware engineers (e.g. Allwinner H700 BSP) frequently write device tree files (`.dts`) assuming the **Xbox layout** (where the top button is Y and the left button is X):

```dts
/* Misnamed DTS node in vendor kernel */
keyX {
    linux,code = <306>; /* BTN_WEST - physically the left button 'Y'! */
};
keyY {
    linux,code = <307>; /* BTN_NORTH - physically the top button 'X'! */
};
```

When Linux initializes `/dev/input/js0`, the buttons are indexed sequentially:
- Event `304` $\to$ `js 0`
- Event `305` $\to$ `js 1`
- Event `306` $\to$ `js 2` (Physical Left button 'Y')
- Event `307` $\to$ `js 3` (Physical Top button 'X')

---

## 2. RetroArch `driver.cfg` Mapping

If you map according to DTS node names (`keyX` $\to$ X), your physical buttons will be inverted!
To ensure:
- Physical **Y** (Left button) triggers Quick Menu (`JOYPAD_Y`)
- Physical **X** (Top button) triggers Translation HUD (`JOYPAD_X`)

You must configure RetroArch to align directly with the electrical wiring:

```ini
# driver.cfg for RG34XX (ANBERNIC-keys)
input_device_p1 = "ANBERNIC-keys"
input_player1_a_btn = "0"
input_player1_b_btn = "1"

# Physical Left button (printed Y) -> sends evdev 306 -> js 2
input_player1_y_btn = "2"

# Physical Top button (printed X) -> sends evdev 307 -> js 3
input_player1_x_btn = "3"
```

---

## 3. Practical Verification Methodology

To verify joystick numbers on any embedded Linux console without guesswork, compile and run this minimal reader:

```c
// gcc -O2 read_js.c -o read_js
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>

int main() {
    int fd = open("/dev/input/js0", O_RDONLY);
    struct js_event e;
    while (read(fd, &e, sizeof(e)) > 0) {
        if (e.type & JS_EVENT_BUTTON) {
            printf("btn=%d val=%d\n", e.number, e.value);
            fflush(stdout);
        }
    }
    return 0;
}
```
Press each physical button on the device to get the exact `js` button index.
