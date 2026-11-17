#!/usr/bin/env python3
"""Regression checks for RetroBilingual pack page identity."""

import hashlib
import importlib.util
import json
import struct
import sqlite3
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
FONT = Path.home() / "development/pocketshell/app/fonts/SarasaUiSC-Regular.ttf"
PACK = ROOT / "tools/pack.py"


def load_pack_module():
    spec = importlib.util.spec_from_file_location("rb_pack", PACK)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def pack_entries(blob: bytes) -> list[tuple[int, int]]:
    assert blob[:4] == b"RBPK"
    _, glyph_w, glyph_h, glyph_count, entry_count = struct.unpack_from("<HHHII", blob, 4)
    glyph_bytes = (glyph_w * glyph_h + 7) // 8
    offset = 18 + 32 + glyph_count * (4 + glyph_bytes)
    return [
        struct.unpack_from("<HB", blob, offset + index * 12)
        for index in range(entry_count)
    ]


@unittest.skipUnless(FONT.exists(), "Sarasa font not available")
class PackPageIdentityTests(unittest.TestCase):
    def test_sanitize_rejects_unmapped_glyph_marker(self):
        with self.assertRaisesRegex(ValueError, "unmapped glyph"):
            load_pack_module().sanitize_text("仍有[G9:E4]字")

    def test_sanitize_makes_known_placeholders_drawable(self):
        self.assertEqual(load_pack_module().sanitize_text("{player} 的档案 {1}"), "林克 的档案 1")

    def test_sanitize_preserves_script_markers_and_blank_lines(self):
        self.assertEqual(load_pack_module().sanitize_text("甲+\n\n乙@"), "甲+\n\n乙@")

    def test_visual_lines_do_not_create_dialogue_pages(self):
        """A source page stays page 0 even when its HUD rendering needs six lines."""
        with tempfile.TemporaryDirectory(prefix="rb-pack-page-") as temporary:
            directory = Path(temporary)
            source_hash = hashlib.sha256(b"page-identity").hexdigest()
            database = directory / "translations.db"
            db = sqlite3.connect(database)
            db.execute(
                "CREATE TABLE translations (game_id TEXT, source_rom_hash TEXT,"
                " source_text_id TEXT, page_index INTEGER, japanese_text TEXT, review_status TEXT)"
            )
            db.execute(
                "INSERT INTO translations VALUES (?, ?, ?, ?, ?, ?)",
                ("golden_sun_jp", source_hash, "0309", 0, "raw", "approved"),
            )
            db.commit()
            db.close()
            texts = directory / "texts.json"
            texts.write_text(
                json.dumps(
                    {"entries": [{
                        "source_id": "0309",
                        "page_index": 0,
                        "text": "第一行\n第二行\n第三行\n第四行\n第五行\n第六行",
                    }]},
                    ensure_ascii=False,
                ),
                encoding="utf-8",
            )
            output = directory / "retrobilingual.pack"
            subprocess.run(
                [
                    "python3", str(PACK),
                    "--database", str(database),
                    "--game-id", "golden_sun_jp",
                    "--texts", str(texts),
                    "--source-rom-sha256", source_hash,
                    "--font", str(FONT),
                    "--output", str(output),
                ],
                check=True,
            )
            self.assertEqual(pack_entries(output.read_bytes()), [(0x0309, 0), (0xFFFF, 0)])


if __name__ == "__main__":
    unittest.main()
