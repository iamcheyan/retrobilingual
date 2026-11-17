# Golden Sun: The Opened Seal (AGSJ01)

This page records the Golden Sun adapter, the version gate, the text/page
model, and the coverage evidence. It intentionally contains no ROM, no
extracted script, and no translation table; those stay in the local,
git-ignored work directory.

## Target versions

| Profile | Region / version | Size | SHA-256 | Runtime role |
| --- | --- | ---: | --- | --- |
| `golden_sun_jp` | Japan, AGSJ01 rev0 (`Ougon no Taiyou - Hirakareshi Fuuin`) | 8,388,608 (`0x00800000`) | `088bedae4bad8b67e87ff10035a898d3639f3182d486fe5a5d113bab223e0a26` | Production target |
| `golden_sun_cn2024` | 2024 public Chinese beta, AGSJ01 base | 11,594,160 (`0x00B0E9B0`) | `f566de927b4ccf95e55686553db3089c26b842158d49a2a91472a8b6ba1419bc` | Local verification only |

The four `.gba` files available in the local resource directory were checked
by header, size, SHA-256, and runtime screenshots. All four are Chinese
translations (CGP classic, CGP & ElfFinal, 2024 public beta, and a renamed
2024/CGP duplicate); none is the clean Japanese AGSJ01 rev0 image. In
particular `golden-sun-gba-2001.gba` is not Japanese despite its AGSJ header.
The clean JP hash above is the version documented by the public Project
Atlantis localization workspace.

## Runtime model

* The current message table ID is a 16-bit value at EWRAM `0x020301D8`.
* `0x0000` / `0xFFFF` means no active message.
* A standalone control `0x01` is a page break inside the same message ID.
* Control `0x11` consumes the following control as a name-buffer operand;
  its `0x01` operand is not a page break.
* Pressing A while a page is still typing fast-forwards. The adapter only
  advances its page counter when the VRAM text tile region
  (`0x06002000`, 1 KiB) has been stable for several frames.
* A page counter that overshoots the built pages (for example while the next
  message loads) is clamped down to the closest existing page by the core
  (`page_slack`). Unknown IDs render nothing; there is no synthetic
  "no translation" entry in the Golden Sun pack.

The page model was validated against the 2024 beta's extracted Japanese
strings: splitting a message on standalone `0x01` yields 9,772 one-page,
1,325 two-page, 9 three-page, 6 four-page, 1 five-page, and 2 six-page
messages. Every translated record produced exactly the same page count as
its Japanese source.

## Text and coverage

The translation source is the public Project Atlantis
`golden-sun-the-broken-seal` workspace, whose records are keyed by the
verified AGSJ01 rev0 text IDs. The local builder consumes its `zh-Hans`
targets, preserves the game's page boundaries, and converts script controls:

* `0x03` / newline -> HUD line break
* `0x11` + operand -> `某某` (the name is visible in the game's own box)
* `0x12` -> `伙伴`, `0x14` -> `技能`, `0x16` -> `道具`
* other controls are removed; they are not visible glyphs

Coverage report (generated locally, not committed with text):

| Metric | Value |
| --- | ---: |
| Message IDs in the table | 11,115 |
| IDs with a built translation | 10,125 |
| Pack entries (per page) | 11,500 |
| Story range | 1,498-11,011 |
| Displayable story IDs | 8,883 |
| Displayable story IDs covered | 8,883 |
| Displayable story uncovered | 0 |
| Non-display story entries | 631 |
| Unique glyphs | 2,490 |
| Page-count mismatches vs Japanese source | 0 |

The 631 non-display story-range IDs are 398 bare `?` debug/unused slots, 230
ASCII/control-only debug labels (for example room tags), 2 single-space
entries, and 1 Djinn-menu fragment (`0x0D3E`, control + grammatical particle)
that is not fired as a dialogue box. The remaining 359 missing IDs are the
same kind of non-display data outside the story range. The seven default
character names (IDs 102-108) are added from the same project's glossary.

The translations are `ai_draft` in the upstream project, not human-reviewed.
This is a quality risk, not a coverage gap; the source text of every record
was independently matched against the Japanese strings decoded from the local
2024 beta (0 mismatches after removing control codes).

### What this pack does not claim

- It does **not** embed a reverse-engineered glyph-code -> Unicode table for
  the 2024 Chinese beta. That ROM's custom Chinese codepage is still not fully
  decoded, and no OCR or model guess was used to fill it. The pack instead uses
  the public Atlantis `zh-Hans` records, which are already keyed by the
  verified Japanese text IDs. This is why the full-coverage claim is about the
  Japanese target's message IDs, not about decoding the Chinese ROM's font.
- Dynamic name/Psynergy/item controls are not resolved at runtime. The HUD
  substitutes `某某` for a name insertion and generic `伙伴` / `技能` / `道具`
  for the corresponding game variables; the game's own box still shows the
  concrete value. The control-code sequence is validated, but the HUD does not
  read the name buffer.
- The Project Atlantis translations are AI drafts. A human-reviewed pass is a
  separate quality task and is not claimed here.

## Building and verifying a pack

```sh
# 1. Build per-page text and a coverage report (local paths are ignored).
python3 tools/golden_sun_build.py \
  --translations /path/to/project-atlantis/games/golden-sun-the-broken-seal/translations \
  --jp-jsonl /path/to/jp-decoded.jsonl \
  --output work/golden-sun/golden-sun-text-zh.json \
  --report work/golden-sun/golden-sun-coverage.json

# 2. Build the pack gated to the clean JP ROM.
python3 tools/pack.py \
  --database work/golden-sun/golden-sun-import.db \
  --game-id golden_sun_jp \
  --texts work/golden-sun/golden-sun-text-zh.json \
  --source-rom-sha256 088bedae4bad8b67e87ff10035a898d3639f3182d486fe5a5d113bab223e0a26 \
  --font "$HOME/development/pocketshell/app/fonts/SarasaUiSC-Regular.ttf" \
  --no-notice \
  --output work/golden-sun/retrobilingual-golden-sun-jp.pack

# 3. Verify header, gate, entries, pages, and glyph coverage.
python3 tools/golden_sun_verify.py \
  --pack work/golden-sun/retrobilingual-golden-sun-jp.pack \
  --texts work/golden-sun/golden-sun-text-zh.json \
  --rom-sha256 088bedae4bad8b67e87ff10035a898d3639f3182d486fe5a5d113bab223e0a26
```

The pack contains only the ROM SHA-256, a 1bpp glyph atlas, and glyph index
sequences. The current JP pack is 604,388 bytes with SHA-256
`097ac0389bd135ceb61f691558e5bbf10a7924c34e4935e7a6bc4281a82a28a1`.

## ROM-size gate

mGBA exposes `GBA.memory.romSize` padded to the cartridge address mask. Golden
Sun's real file sizes (`0x8D4000`, `0xB0E9B0`) are not powers of two, so the
core hashes and matches on `GBA.pristineRomSize` when available. Without this,
the adapter and pack gate can never match on a real mGBA core even though the
host test (which passes the exact file size) succeeds.

## Tests

```sh
make test
python3 tests/test-retrobilingual-host.py    # PJOS tree
python3 tests/test-retrobilingual-pack.py    # PJOS tree
sh tools/build-retrobilingual-mgba-h700.sh   # PJOS AArch64 core
```

The host harness covers the exact JP and 2024 beta ROM gates, EWRAM message
ID reads, multi-page A-press sync, typewriter fast-forward rejection, message
change resets, closed-box reset, unknown IDs, and a mismatched pack gate. The
real-ROM verification profile loads the 2024 beta and renders its current
message through the full pack; it is a verification aid, not the production
target.

## Deployment

The device install path is `/storage/.config/pocketjs/retrobilingual.pack`.
Use the PJOS helper with the pack built for the **clean JP** ROM:

```sh
PJOS_ADB_SERIAL=<serial> sh tools/deploy-retrobilingual.sh \
  --pack work/golden-sun/retrobilingual-golden-sun-jp.pack
```

The helper backs up the existing core and driver, pushes the pack atomically,
and verifies the core hash. It does not start or stop a game.

## Open external dependencies

1. The clean Japanese AGSJ01 rev0 ROM (size/hash above) is not present on this
   machine. The adapter's JP gate and pack are built for it, but its runtime
   EWRAM cursor and page behavior cannot be exercised until that exact dump is
   supplied.
2. No authorized ADB/SSH session to the RG34XX was available during this work
   (`adb devices` empty; the reachable LAN SSH endpoint rejected the available
   credentials). Device deployment and framebuffer verification are therefore
   not claimed.
