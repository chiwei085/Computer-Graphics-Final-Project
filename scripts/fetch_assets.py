#!/usr/bin/env python3
"""Curated open-source asset fetcher.

Quality bar: we do NOT hand-model the hero AI props. Instead we pull a small,
deliberately curated set of license-clear, high-recognition models and convert
them to clean OBJ. Selection criteria live in
``docs/dev/plan_step/README.md`` (the curation philosophy section).

The MANIFEST below is the vetted shortlist. Entries with ``auto=True`` are
fetched directly over HTTPS; entries with ``auto=False`` document a manual
source (e.g. itch.io / Google Drive) that cannot be scripted cleanly and must
be downloaded by hand — they are listed so the choice is recorded, not lost.
"""

from __future__ import annotations

import hashlib
import re
import urllib.request
from pathlib import Path
from typing import Annotated

import typer

REPO_ROOT = Path(__file__).resolve().parent.parent
RAW_DIR = REPO_ROOT / "assets" / "raw"
CREDITS_PATH = REPO_ROOT / "assets" / "CREDITS.md"

MANIFEST: dict[str, dict] = {
    "robonaut2": {
        "role": "主要 AI 外部 OBJ — 人形服務機器人。NASA 真實工程機體，"
                "辨識度最高且與晚餐桌『人 vs 機器』敘事最契合。",
        "filename": "Robonaut 2.glb",
        "url": "https://raw.githubusercontent.com/nasa/NASA-3D-Resources/"
               "master/3D%20Models/Robonaut%202/Robonaut%202.glb",
        "license": "NASA media — public domain (see NASA usage guidelines)",
        "attribution": "NASA / NASA-3D-Resources (Robonaut 2)",
        "format": "glb -> convert to OBJ",
        "auto": True,
    },
    "ingenuity": {
        "role": "Orbit 主角 — 自主旋翼無人機。真實火星直升機，旋翼造型適合繞巨眼公轉。",
        "filename": "Ingenuity Mars Helicopter.glb",
        "url": "https://raw.githubusercontent.com/nasa/NASA-3D-Resources/"
               "master/3D%20Models/Ingenuity%20Mars%20Helicopter/"
               "Ingenuity%20Mars%20Helicopter.glb",
        "license": "NASA media — public domain (see NASA usage guidelines)",
        "attribution": "NASA / NASA-3D-Resources (Ingenuity Mars Helicopter)",
        "format": "glb -> convert to OBJ",
        "auto": True,
    },
    "infrared_camera": {
        "role": "預見區 — 桌面預測投影器 / 感測鏡頭。補強『AI 正在計算未來』的硬體來源。",
        "filename": "Infrared Camera.glb",
        "url": "https://raw.githubusercontent.com/nasa/NASA-3D-Resources/"
               "master/3D%20Models/Infrared%20Camera/Infrared%20Camera.glb",
        "license": "NASA media — public domain (see NASA usage guidelines)",
        "attribution": "NASA / NASA-3D-Resources (Infrared Camera)",
        "format": "glb -> convert to OBJ",
        "auto": True,
    },
    "astronaut_glove": {
        "role": "眷戀區 — 空椅旁的人類殘痕。以真實手套暗示曾經在場的人與記憶重量。",
        "filename": "Astronaut Glove.glb",
        "url": "https://raw.githubusercontent.com/nasa/NASA-3D-Resources/"
               "master/3D%20Models/Astronaut%20Glove/Astronaut%20Glove.glb",
        "license": "NASA media — public domain (see NASA usage guidelines)",
        "attribution": "NASA / NASA-3D-Resources (Astronaut Glove)",
        "format": "glb -> convert to OBJ",
        "auto": True,
    },
    "crew_lock_bag": {
        "role": "盲點區 — 桌下被忽略的收納袋。維持完整實體，不被預測或眷戀特效吞掉。",
        "filename": "Crew Lock Bag.glb",
        "url": "https://raw.githubusercontent.com/nasa/NASA-3D-Resources/"
               "master/3D%20Models/Crew%20Lock%20Bag/Crew%20Lock%20Bag.glb",
        "license": "NASA media — public domain (see NASA usage guidelines)",
        "attribution": "NASA / NASA-3D-Resources (Crew Lock Bag)",
        "format": "glb -> convert to OBJ",
        "auto": True,
    },
    "dirty_plate": {
        "role": "預見區 — 事故鏈中的滑動髒盤，讓預測結果有實體餐桌後果。",
        "filename": "Dirty Plate.glb",
        "url": "https://poly.pizza/m/XJfPNgGbMl",
        "license": "Public Domain (CC0)",
        "attribution": "Kay Lousberg / Poly Pizza (Dirty Plate)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "bowl_dirty": {
        "role": "預見區 — 被撞歪的髒碗，作為酒杯事故的第二個未來分支。",
        "filename": "Bowl Dirty.glb",
        "url": "https://poly.pizza/m/ASUKhSq7pS",
        "license": "Public Domain (CC0)",
        "attribution": "Kay Lousberg / Poly Pizza (Bowl Dirty)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "stew": {
        "role": "預見區 — 湯/燉菜即將灑出的具體食物結果。",
        "filename": "Stew.glb",
        "url": "https://poly.pizza/m/rPa4vEsC9c",
        "license": "Public Domain (CC0)",
        "attribution": "Kay Lousberg / Poly Pizza (Stew)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "cutting_board": {
        "role": "預見區 — 事故連鎖中被推移的砧板/硬物。",
        "filename": "Cutting Board.glb",
        "url": "https://poly.pizza/m/S6GVjVtk0J",
        "license": "Public Domain (CC0)",
        "attribution": "Kay Lousberg / Poly Pizza (Cutting Board)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "chopsticks": {
        "role": "預見區 — 被撞散的餐具，讓未來分支不只靠光線表示。",
        "filename": "Chopsticks.glb",
        "url": "https://poly.pizza/m/UOhOqQvF9y",
        "license": "Public Domain (CC0)",
        "attribution": "Quaternius / Poly Pizza (Chopsticks)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "present": {
        "role": "眷戀區 — 左椅記憶祭壇的禮物盒，暗示未說完的關係。",
        "filename": "Present.glb",
        "url": "https://poly.pizza/m/LVg3ynJDxa",
        "license": "Public Domain (CC0)",
        "attribution": "CreativeTrio / Poly Pizza (Present)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "wall_corkboard": {
        "role": "眷戀區 — 便條板/照片牆，讓私人記憶有可讀文字/照片介面。",
        "filename": "Wall Corkboard.glb",
        "url": "https://poly.pizza/m/U8yQZ9l0HZ",
        "license": "Public Domain (CC0)",
        "attribution": "CreativeTrio / Poly Pizza (Wall Corkboard)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "mug_office_tool": {
        "role": "眷戀區 — 杯中筆具，補中椅孩子堡壘/家庭日常的私人細節。",
        "filename": "Mug With Office Tool.glb",
        "url": "https://poly.pizza/m/4jSgnM5WWk",
        "license": "Public Domain (CC0)",
        "attribution": "CreativeTrio / Poly Pizza (Mug With Office Tool)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "closed_umbrella": {
        "role": "眷戀區 — 右椅爭吵後留下的雨傘/離席痕跡。",
        "filename": "Closed Umbrella.glb",
        "url": "https://poly.pizza/m/o0CUgpt8pm",
        "license": "Public Domain (CC0)",
        "attribution": "CreativeTrio / Poly Pizza (Closed Umbrella)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "zz_plant": {
        "role": "盲點區 — 桌下盆栽/生命痕跡，完整但不被 AI 解釋。",
        "filename": "Zz Plant.glb",
        "url": "https://poly.pizza/m/bTRzVhywtU",
        "license": "Public Domain (CC0)",
        "attribution": "Isa Lousberg / Poly Pizza (Zz Plant)",
        "format": "Poly Pizza page -> GLB -> convert to OBJ",
        "auto": True,
        "source": "poly_pizza",
    },
    "quaternius_scifi": {
        "role": "風格化備援 — 若需要更乾淨 game-ready 拓樸的 sci-fi 機器人。"
                "僅在 NASA 機體拓樸不利著色時啟用。",
        "filename": "(manual download)",
        "url": "https://quaternius.com/  (Sci-Fi / Animated Robot packs)",
        "license": "CC0",
        "attribution": "Quaternius (CC0)",
        "format": "native OBJ/FBX/glTF",
        "auto": False,
    },
}

app = typer.Typer(help="Fetch curated open-source 3D assets.")


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def _fetch_one(key: str, meta: dict) -> Path | None:
    if not meta.get("auto"):
        typer.echo(f"[skip] {key}: manual source — {meta['url']}")
        return None
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    dest = RAW_DIR / meta["filename"]
    if dest.exists() and dest.stat().st_size > 0:
        typer.echo(f"[have] {key}: {dest.name} ({dest.stat().st_size} bytes)")
        return dest
    url = meta["url"]
    if meta.get("source") == "poly_pizza":
        typer.echo(f"[page] {key}: {url}")
        page_req = urllib.request.Request(
            url, headers={"User-Agent": "Mozilla/5.0"}
        )
        try:
            with urllib.request.urlopen(page_req, timeout=60) as resp:
                page = resp.read().decode("utf-8", "replace")
        except Exception as exc:
            typer.echo(f"[FAIL] {key}: {exc}", err=True)
            return None
        match = re.search(r"https://static\.poly\.pizza/[^\"&]+\.glb", page)
        if match is None:
            typer.echo(f"[FAIL] {key}: no GLB URL found on Poly Pizza page", err=True)
            return None
        url = match.group(0)

    typer.echo(f"[get ] {key}: {url}")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp, dest.open("wb") as out:
            out.write(resp.read())
    except Exception as exc:
        typer.echo(f"[FAIL] {key}: {exc}", err=True)
        return None
    typer.echo(f"[ok  ] {key}: {dest.name} ({dest.stat().st_size} bytes)")
    return dest


def _write_credits(fetched: dict[str, Path | None]) -> None:
    CREDITS_PATH.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Asset Credits & Licenses", "",
        "外部資產出處與授權。所有外部模型皆為 public domain / CC0，",
        "並記錄出處以利報告引用。", "",
    ]
    for key, meta in MANIFEST.items():
        got = fetched.get(key)
        sha = _sha256(got) if got and got.exists() else "—"
        lines += [
            f"## {key}",
            f"- 角色: {meta['role']}",
            f"- 授權: {meta['license']}",
            f"- 出處: {meta['attribution']}",
            f"- 來源: {meta['url']}",
            f"- 格式: {meta['format']}",
            f"- 檔案: {meta['filename']}",
            f"- sha256: `{sha}`",
        ]
        if "video_url" in meta:
            lines.append(f"- 原始影片: {meta['video_url']}")
        if "notes" in meta:
            lines.append(f"- 備註: {meta['notes']}")
        lines.append("")
    lines += [
        "## textures",
        "",
        "全部 6 張貼圖由 `scripts/gen_textures.py` 以程序生成，屬自製素材，CC0 釋出。",
        "",
        "| 檔案 | 用途 | 技術 |",
        "|------|------|------|",
        "| `wood.png` | 桌面 + 桌腳 | 正弦波木紋條紋 |",
        "| `cloth.png` | 椅座 + 椅背 | 週期性編織格紋 |",
        "| `circuit.png` | 巨眼虹膜電路環 (AI 物件必要項) | PCB 走線 + 焊盤隨機分佈 |",
        "| `marble.png` | 鞏膜球 + 瓷盤 | 正弦波大理石紋 |",
        "| `metal.png` | 虹膜框 + 縫隙環 + 線纜 | 水平磨砂拉絲 |",
        "| `paper.png` | 桌上信封 | 白紙纖維噪點 |",
        "",
        "- 授權: CC0 (自製，無版權限制)",
        "- 生成腳本: `scripts/gen_textures.py`",
        "- 尺寸: 512×512 PNG",
        "",
    ]
    CREDITS_PATH.write_text("\n".join(lines), encoding="utf-8")
    typer.echo(f"[ok  ] wrote {CREDITS_PATH.relative_to(REPO_ROOT)}")


@app.command("list")
def cmd_list() -> None:
    """Print the curated shortlist."""
    for key, meta in MANIFEST.items():
        tag = "auto" if meta.get("auto") else "manual"
        typer.echo(f"[{tag}] {key}: {meta['role']}")


@app.command("fetch")
def cmd_fetch(
    keys: Annotated[
        list[str] | None,
        typer.Argument(help="Keys to fetch (default: all auto entries)."),
    ] = None,
) -> None:
    """Download assets and write assets/CREDITS.md."""
    targets = keys or list(MANIFEST)
    unknown = [k for k in targets if k not in MANIFEST]
    if unknown:
        typer.echo(f"unknown keys: {unknown}", err=True)
        raise typer.Exit(1)

    fetched: dict[str, Path | None] = {}
    for key in targets:
        fetched[key] = _fetch_one(key, MANIFEST[key])
    _write_credits(fetched)
    typer.echo("\nNext: uv run python convert_glb_to_obj.py")


if __name__ == "__main__":
    app()
