# RetroBilingual

<p align="center">
  <b>In-Frame Bilingual Translation Overlay & Quick-Action HUD for Retro Game Emulators</b><br>
  <i>嵌入式复古掌机 / 模拟器游戏即时双语字幕与快捷操作 HUD 引擎</i>
</p>

---

## 🌟 Overview / 项目简介

**RetroBilingual** is an open-source, in-core translation and utility framework for retro game emulators (Libretro / GBA).

Traditional game translation approaches on handhelds suffer from severe limitations:
- **Screen OCR** is CPU-intensive, introduces hundred-millisecond latencies, drains battery on ARM SoCs, and struggles with small pixel fonts.
- **Timeline subtitles** inevitably drift and desynchronize because retro game dialogues are driven by player input and variable reading speed.

**RetroBilingual solves this fundamentally through in-frame RAM reflection:**
By listening directly to the emulator's memory bus (e.g. GBA EWRAM), the core detects the exact dialogue ID and text rendering state machine in real time. Whenever the player requests a translation, it instantly performs an $O(\log N)$ binary lookup in a compact offline dictionary pack and renders a clean, non-intrusive floating HUD onto the game's framebuffer before display output.

---

## ✨ Features / 核心特性

- **Zero-Latency In-Frame Rendering (零延迟同帧直写)**:
  Renders directly onto the emulator's native RGB565 framebuffer before the video callback. No external GUI windows, no `/dev/fb0` multi-process contention, no screen tearing.
- **Memory-Reflection Dialogue Alignment (运行内存反射对齐)**:
  Extracts the live `textIndex` and sub-page counter directly from game RAM. Even if you skip 50 dialogue lines without checking translation, opening it at any time always displays the exact sentence currently shown on screen.
- **Adaptive Multi-Line Floating HUD (智能无感双行浮层)**:
  Minimalist translucent backing with high-contrast 1bpp bitmap font. Supports 1-line and 2-line standard GBA dialogue boxes, with configurable vertical placement (`AUTO`, `TOP`, `CENTER`, `BOTTOM`).
- **5-Slot Instant Save State Management (5 槽位即时存档与状态感知)**:
  Built-in quick save/load system decoupled from default emulator slots. Features live filesystem detection (populated slots highlighted in green, empty in gray) and smooth on-screen Toast alerts.
- **Modular Game Adapter Architecture (模块化多游戏适配器架构)**:
  While shipped with *The Legend of Zelda: The Minish Cap* as the reference implementation, the core engine is **100% game-agnostic**. Any retro game can be supported simply by adding a small RAM adapter.
- **Embedded Font Atlas Compiler (超紧凑字模打包器)**:
  Python CLI tool `pack.py` automatically extracts and bakes only the glyphs used in your translation into a 12px 1bpp atlas, reducing memory consumption to negligible levels.

---

## 🕹️ Controls (Default RG34XX / RetroPad Mapping)

| Key | Action | Description |
|---|---|---|
| **Y (Left button)** | **Quick Menu (快捷菜单)** | Opens in-game overlay menu (Resume, Quick Save, Quick Load, X Action, Position). Pauses game logic. |
| **X (Top button)** | **Instant Translate (一键翻译)** | Immediately pops up floating translation HUD for current line. Tap X or B to dismiss. |
| **B (Bottom button)**| **Back / Resume (返回/恢复)** | Closes open menu or subtitle HUD and unfreezes the game. |
| **D-Pad Up/Down** | **Navigate Menu** | Cycles menu items. |
| **D-Pad Left/Right**| **Cycle Slot / Position** | Switches save slot (1..5) or HUD position (`AUTO` / `TOP` / `CENTER` / `BOTTOM`). |

---

## 📁 Repository Structure / 目录结构

```text
retrobilingual/
├── README.md               # Project documentation & overview
├── LICENSE                 # MIT License
├── core/                   # C Core Engine
│   ├── retrobilingual.c    # Universal HUD, menu, save states, font blitter
│   ├── retrobilingual.h    # Public Libretro hooks
│   └── adapters/           # Modular game RAM reflection adapters
│       ├── adapter.h       # Generic adapter interface
│       ├── tmc.c           # Reference: Zelda Minish Cap adapter
│       └── template.c      # Boilerplate template for new games
├── patches/                # Integration hooks
│   └── apply-mgba.py       # One-step patch script for mGBA
├── tools/                  # Build & dataset tools
│   └── pack.py             # CLI compiler: JSON + TTF -> retrobilingual.pack
├── docs/                   # In-depth technical guides
│   ├── ARCHITECTURE.md     # Architectural deep dive
│   ├── HOW_TO_ADAPT_GAMES.md # Complete tutorial: adapting any new game
│   └── HARDWARE_MAPPING.md # Embedded Linux button mapping & DTS debugging
└── tests/                  # Standalone test suite
    ├── test_host.py        # Host Python test runner
    └── host/               # Mock emulator environment & C tests
```

---

## 🚀 Quickstart / 快速上手

### 1. Run Host Test Suite (无需掌机，直接在电脑运行单元测试)
```bash
python3 tests/test_host.py
```
*Outputs: `host overlay test: pack gate, page tracking, chord pause, subtitle, 5 slots, position cycling, save/load toasts OK`.*

### 2. Build a Translation Pack for Your Game (打包字典)
```bash
python3 tools/pack.py \
  --texts /path/to/translations.json \
  --source-rom-sha256 <64_character_hex_hash> \
  --font /path/to/any_truetype_font.ttf \
  --font-size 12 \
  --output retrobilingual.pack
```

### 3. Integrate into mGBA (集成到 mGBA 模拟器)
```bash
python3 patches/apply-mgba.py /path/to/mgba /path/to/retrobilingual/core
```

---

## 📖 Documentation / 进阶文档

- [Architecture & Mechanics (架构原理解析)](docs/ARCHITECTURE.md)
- [How to Adapt Any Retro Game (如何为任意复古游戏制作双语适配)](docs/HOW_TO_ADAPT_GAMES.md)
- [Handheld Hardware Mapping & DTS Pitfalls (掌机按键驱动与硬件映射排错指南)](docs/HARDWARE_MAPPING.md)

---

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
