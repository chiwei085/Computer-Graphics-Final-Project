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
    typer.echo(f"[get ] {key}: {meta['url']}")
    req = urllib.request.Request(meta["url"], headers={"User-Agent": "curl/8"})
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
    typer.echo("\nNext: uv run python scripts/convert_glb_to_obj.py")


if __name__ == "__main__":
    app()
