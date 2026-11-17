#!/usr/bin/env python3
"""Compile and run the host overlay harness against a synthetic approved pack."""
import hashlib
import importlib.util
import json
import os
import sqlite3
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[1]
FONT = Path.home() / "development/pocketshell/app/fonts/SarasaUiSC-Regular.ttf"
PACK_PATH = ROOT / "tools/pack.py"


def load_pack_module():
    spec = importlib.util.spec_from_file_location("rpack", PACK_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    if not FONT.exists():
        print("skip: Sarasa font not available")
        return 0
    pack = load_pack_module()
    with tempfile.TemporaryDirectory(prefix="rb-host-") as temporary:
        tmp = Path(temporary)
        rom = bytes(16 * 1024 * 1024)
        (tmp / "rom.gba").write_bytes(rom)
        digest = hashlib.sha256(rom).hexdigest()

        db = sqlite3.connect(tmp / "translations.db")
        db.execute(
            "CREATE TABLE translations (mapping_id INTEGER PRIMARY KEY, game_id TEXT,"
            " source_rom_hash TEXT, source_text_id TEXT, context_id TEXT, page_index INTEGER,"
            " japanese_text TEXT, chinese_text TEXT, match_method TEXT, confidence REAL, review_status TEXT)"
        )
        for page, text in ((0, "你好世界"), (1, "第二页")):
            db.execute(
                "INSERT INTO translations VALUES (?,?,?,?,?,?,?,?,?,?,?)",
                (page + 1, "zelda_minish_cap", digest, "0201", "", page, "raw", "raw", "test", 1.0, "approved"),
            )
        db.commit()
        db.close()

        selected = [
            ("0201", 0, "一"),
            ("0201", 1, "二三四"),
            ("0202", 0, "甲乙丙"),
            ("0203", 0, "丁戊"),
            ("0203", 2, "己庚辛"),
            ("0204", 0, "一\n二\n三\n四\n五\n六"),
            ("FFFF", 0, "暂无翻译"),
        ]
        chars = {c for _, _, text in selected for c in text}
        atlas, index = pack.build_atlas(chars, FONT, 12)
        blob = pack.build_pack(selected, atlas, index, 12, bytes.fromhex(digest))
        (tmp / "retrobilingual.pack").write_bytes(blob)

        binary = tmp / "overlay-test"
        subprocess.run(
            [
                "gcc", "-std=c11", "-O1", "-Wall", "-DRB_HOST_TEST",
                "-I", str(ROOT / "tests/host"), "-I", str(ROOT / "core"),
                str(ROOT / "core/retrobilingual.c"),
                str(ROOT / "tests/host/test_adapter.c"),
                str(ROOT / "tests/host/test_overlay.c"),
                "-o", str(binary),
            ],
            check=True,
        )
        env = dict(os.environ, RETROBILINGUAL_DIR=str(tmp))
        result = subprocess.run([str(binary), str(tmp / "rom.gba")], env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        print(result.stdout.strip())
        if result.returncode != 0:
            print("host overlay test failed")
            return result.returncode

        golden_sun = run_golden_sun(tmp, pack)
        if golden_sun:
            return golden_sun
        return 0


GOLDEN_SUN_ROM = (
    Path.home() / "development/PJOS-LINUX/resouces/GBA模拟器[PC端用]"
    / "黄金太阳1-开启的封印（2024 new CN）.gba"
)


def run_golden_sun(tmp: Path, pack) -> int:
    if not GOLDEN_SUN_ROM.exists():
        print("skip: Golden Sun ROM not available")
        return 0
    rom = GOLDEN_SUN_ROM.read_bytes()
    digest = hashlib.sha256(rom).hexdigest()
    selected = [
        ("0FDD", 0, "测试文本第一行\n测试文本第二行"),
        ("0B37", 0, "翻页测试第一页第一行\n翻页测试第一页第二行"),
        ("0B37", 1, "翻页测试第二页"),
        ("0B38", 0, "重置测试文本第一行\n重置测试文本第二行"),
    ]
    chars = {c for _, _, text in selected for c in text if c != "\n"}
    atlas, index = pack.build_atlas(chars, FONT, 12)
    blob = pack.build_pack(selected, atlas, index, 12, bytes.fromhex(digest))
    golden_dir = tmp / "golden-sun"
    golden_dir.mkdir()
    (golden_dir / "retrobilingual.pack").write_bytes(blob)

    binary = tmp / "golden-sun-test"
    subprocess.run(
        [
            "gcc", "-std=c11", "-O1", "-Wall", "-DRB_HOST_TEST",
            "-I", str(ROOT / "tests/host"), "-I", str(ROOT / "core"),
            str(ROOT / "core/retrobilingual.c"),
            str(ROOT / "core/adapters/golden_sun.c"),
            str(ROOT / "core/adapters/tmc.c"),
            str(ROOT / "core/adapters/registry.c"),
            str(ROOT / "tests/host/test_golden_sun.c"),
            "-o", str(binary),
        ],
        check=True,
    )
    env = dict(os.environ, RETROBILINGUAL_DIR=str(golden_dir))
    result = subprocess.run([str(binary), str(GOLDEN_SUN_ROM)], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(result.stdout.strip())
    if result.returncode != 0:
        print("golden sun host test failed")
        return result.returncode

    wrong = bytearray(blob)
    wrong[18] ^= 0xFF
    (golden_dir / "retrobilingual.pack").write_bytes(wrong)
    result = subprocess.run([str(binary), str(GOLDEN_SUN_ROM), "bad-gate"], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(result.stdout.strip())
    if result.returncode != 0:
        print("golden sun ROM gate test failed")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
