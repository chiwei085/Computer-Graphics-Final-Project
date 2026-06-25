#!/usr/bin/env python3
"""
observe_app.py — Build, launch, and interact with the Future's Gaze OpenGL app.

Exposes enough information for AI-driven testing and observation:
  screenshot  - capture window + per-region RGB/brightness analysis
  info        - full diagnostic dump (process, window geometry, pixel analysis)
  build       - cmake configure + ninja build
  launch      - start app in background, save PID
  kill        - terminate running app
  orbit       - left-mouse drag to rotate camera (dx, dy in pixels)
  zoom        - mouse-wheel scroll to zoom camera
  key         - send keyboard key to the app window
  click       - click at window-relative coordinates (fractions 0.0-1.0)

Usage:
  uv run python observe_app.py info
  uv run python observe_app.py launch --build
  uv run python observe_app.py screenshot --out /tmp/frame.png
  uv run python observe_app.py orbit 200 -50
  uv run python observe_app.py zoom 3
  uv run python observe_app.py key escape
"""

from __future__ import annotations

import os
import re
import signal
import subprocess
import time
from pathlib import Path
from typing import Annotated

import typer

# ── Constants ─────────────────────────────────────────────────────────────────

PROJECT_ROOT = Path(__file__).parent.parent
BUILD_DIR = PROJECT_ROOT / "build" / "clang-debug"
BINARY = BUILD_DIR / "future_gaze"
WINDOW_TITLE = "Future's Gaze"
PID_FILE = Path("/tmp/future_gaze.pid")
SCREENSHOT_DIR = PROJECT_ROOT / "docs" / "screenshots"

cli = typer.Typer(
    help="Interact with Future's Gaze OpenGL window.",
    no_args_is_help=True,
)

# ── Display / auth helpers ─────────────────────────────────────────────────────


def _ensure_display() -> None:
    if not os.environ.get("DISPLAY"):
        os.environ["DISPLAY"] = ":0"
    # python-xlib requires ~/.Xauthority to exist; an empty file = "no auth"
    # which is correct for WSLg's open local X server.
    xauth = Path.home() / ".Xauthority"
    if not xauth.exists():
        xauth.touch()


# ── Lazy GUI imports (need a live X display) ──────────────────────────────────


def _pag():
    """Return pyautogui, importing lazily so --help works without a display."""
    _ensure_display()
    try:
        import pyautogui

        pyautogui.PAUSE = 0.04
        pyautogui.FAILSAFE = False
        return pyautogui
    except Exception as exc:
        typer.echo(
            f"✗ pyautogui unavailable (DISPLAY={os.environ.get('DISPLAY', 'unset')}): {exc}",
            err=True,
        )
        raise typer.Exit(1) from exc


def _pil():
    """Return (Image, ImageStat) from Pillow."""
    from PIL import Image, ImageStat

    return Image, ImageStat


def _ease_in_out_quad(t: float) -> float:
    """Quadratic ease-in/out curve for pyautogui tween callbacks."""
    if t < 0.5:
        return 2.0 * t * t
    return -1.0 + (4.0 - 2.0 * t) * t


# ── Window discovery ───────────────────────────────────────────────────────────


def _find_window_xdotool() -> tuple[int, int, int, int] | None:
    """Return (x, y, w, h) via xdotool (preferred when installed)."""
    try:
        result = subprocess.run(
            ["xdotool", "search", "--name", WINDOW_TITLE, "--onlyvisible"],
            capture_output=True,
            text=True,
            timeout=3,
        )
        win_ids = result.stdout.strip().split()
        if not win_ids:
            return None
        win_id = win_ids[-1]
        geo = subprocess.run(
            ["xdotool", "getwindowgeometry", "--shell", win_id],
            capture_output=True,
            text=True,
            timeout=3,
        )
        info: dict[str, int] = {}
        for line in geo.stdout.splitlines():
            k, _, v = line.partition("=")
            if v.strip().lstrip("-").isdigit():
                info[k.strip()] = int(v.strip())
        return info["X"], info["Y"], info["WIDTH"], info["HEIGHT"]
    except (FileNotFoundError, subprocess.TimeoutExpired, KeyError, ValueError):
        return None


def _find_window_xwininfo() -> tuple[int, int, int, int] | None:
    """Return (x, y, w, h) by scanning the root window tree via xwininfo.

    Uses partial title match so it works even if the app appends a suffix like
    "- P3" to WINDOW_TITLE.  First finds the window ID from the tree, then
    queries its exact geometry.
    """
    env = {**os.environ, "DISPLAY": os.environ.get("DISPLAY", ":0")}
    try:
        # Step 1: find window ID via root tree (partial title match)
        tree = subprocess.run(
            ["xwininfo", "-root", "-tree"],
            capture_output=True,
            text=True,
            timeout=5,
            env=env,
        )
        win_id: str | None = None
        for line in tree.stdout.splitlines():
            if WINDOW_TITLE in line:
                m = re.search(r"(0x[0-9a-fA-F]+)", line)
                if m:
                    win_id = m.group(1)
                    break
        if win_id is None:
            return None

        # Step 2: query geometry for that window ID
        geo = subprocess.run(
            ["xwininfo", "-id", win_id],
            capture_output=True,
            text=True,
            timeout=5,
            env=env,
        )
        info: dict[str, int] = {}
        for line in geo.stdout.splitlines():
            for key, pattern in [
                ("X", "Absolute upper-left X:"),
                ("Y", "Absolute upper-left Y:"),
                ("WIDTH", "Width:"),
                ("HEIGHT", "Height:"),
            ]:
                if pattern in line:
                    try:
                        info[key] = int(line.split(":")[-1].strip())
                    except ValueError:
                        pass
        if len(info) == 4:
            return info["X"], info["Y"], info["WIDTH"], info["HEIGHT"]
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return None


def _find_window() -> tuple[int, int, int, int] | None:
    """Return (x, y, w, h) of the app window, trying xdotool then xwininfo."""
    return _find_window_xdotool() or _find_window_xwininfo()


def _find_window_id_xwininfo() -> str | None:
    """Return the X window ID (hex string) by partial title match."""
    env = {**os.environ, "DISPLAY": os.environ.get("DISPLAY", ":0")}
    try:
        tree = subprocess.run(
            ["xwininfo", "-root", "-tree"],
            capture_output=True,
            text=True,
            timeout=5,
            env=env,
        )
        for line in tree.stdout.splitlines():
            if WINDOW_TITLE in line:
                m = re.search(r"(0x[0-9a-fA-F]+)", line)
                if m:
                    return m.group(1)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return None


def _window_center() -> tuple[int, int]:
    geo = _find_window()
    if geo:
        x, y, w, h = geo
        return x + w // 2, y + h // 2
    sw, sh = _pag().size()
    return sw // 2, sh // 2


def _require_window() -> tuple[int, int, int, int]:
    geo = _find_window()
    if geo is None:
        typer.echo("✗ Window not found. Is the app running?", err=True)
        raise typer.Exit(1)
    return geo


# ── Screenshot helpers ─────────────────────────────────────────────────────────


def _screenshot_xwd(out_path: Path):
    """Capture window via xwd | ffmpeg → PNG.

    Uses the window ID to avoid exact-title matching issues.
    Returns a PIL Image on success, None on failure.
    xwd captures exact GL framebuffer content (no WSLg coordinate offset).
    """
    env = {**os.environ, "DISPLAY": os.environ.get("DISPLAY", ":0")}
    win_id = _find_window_id_xwininfo()
    if win_id is None:
        typer.echo("⚠ xwd: window not found", err=True)
        return None
    try:
        xwd_proc = subprocess.Popen(
            ["xwd", "-id", win_id, "-silent"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            env=env,
        )
        ff = subprocess.run(
            ["ffmpeg", "-y", "-i", "pipe:0", str(out_path)],
            stdin=xwd_proc.stdout,
            capture_output=True,
            timeout=10,
        )
        xwd_proc.stdout.close()  # type: ignore[union-attr]
        xwd_proc.wait(timeout=5)
        if ff.returncode == 0 and out_path.exists():
            from PIL import Image

            return Image.open(str(out_path))
        stderr_tail = ff.stderr[-200:].decode(errors="replace")
        typer.echo(f"⚠ ffmpeg decode failed (rc={ff.returncode}): {stderr_tail}", err=True)
    except Exception as exc:
        typer.echo(f"⚠ xwd capture failed: {exc}", err=True)
    return None


# ── Commands ───────────────────────────────────────────────────────────────────


@cli.command()
def build() -> None:
    """Configure and build the project (cmake --preset clang-debug + ninja)."""
    typer.echo("→ cmake configure...")
    subprocess.run(
        ["cmake", "--preset", "clang-debug"],
        cwd=PROJECT_ROOT,
        check=True,
    )
    typer.echo("→ ninja build...")
    subprocess.run(
        ["ninja", "-C", str(BUILD_DIR)],
        check=True,
    )
    typer.echo(f"✓ Build complete: {BINARY}")


@cli.command()
def launch(
    do_build: bool = typer.Option(False, "--build", "-b", help="Build before launching."),
    wait: float = typer.Option(2.0, "--wait", help="Seconds to wait for window after launch."),
) -> None:
    """Launch the app in the background. PID saved to /tmp/future_gaze.pid."""
    if do_build:
        build()

    if not BINARY.exists():
        typer.echo(f"✗ Binary not found: {BINARY}. Run `build` first.", err=True)
        raise typer.Exit(1)

    _ensure_display()
    env = os.environ.copy()

    proc = subprocess.Popen(
        [str(BINARY)],
        cwd=PROJECT_ROOT,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    PID_FILE.write_text(str(proc.pid))
    typer.echo(f"✓ Launched PID={proc.pid}. Waiting {wait}s for window...")
    time.sleep(wait)

    geo = _find_window()
    if geo:
        x, y, w, h = geo
        typer.echo(f"✓ Window found: {w}×{h} at screen ({x},{y})")
    else:
        typer.echo("⚠ Window not found (may still be starting up)")


@cli.command()
def kill() -> None:
    """Send SIGTERM to the running app."""
    if not PID_FILE.exists():
        typer.echo("✗ No PID file. App not launched via this script?", err=True)
        raise typer.Exit(1)
    pid = int(PID_FILE.read_text().strip())
    try:
        os.kill(pid, signal.SIGTERM)
        typer.echo(f"✓ SIGTERM → PID {pid}")
    except ProcessLookupError:
        typer.echo(f"⚠ PID {pid} not found (already exited?)")
    PID_FILE.unlink(missing_ok=True)


@cli.command()
def screenshot(
    out: Annotated[
        Path | None,
        typer.Option("--out", "-o", help="Output PNG path."),
    ] = None,
    no_analysis: Annotated[
        bool,
        typer.Option("--no-analysis", help="Skip region pixel analysis."),
    ] = False,
) -> None:
    """
    Capture the app window to PNG and print pixel-level region analysis.

    Uses xwd | ffmpeg for reliable GL framebuffer capture (no WSLg offset).
    Falls back to pyautogui if xwd/ffmpeg is unavailable.

    The analysis divides the window into 9 zones and reports mean RGB +
    brightness so AI can detect rendering regressions:
      all-black = crash or clear-color failure
      unexpected bright region = lighting/geometry artifact
    """
    SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
    ts = int(time.time())
    out_path = out or (SCREENSHOT_DIR / f"screenshot_{ts}.png")

    _ensure_display()

    # Preferred: xwd | ffmpeg (correct GL content, no WSLg coordinate offset)
    img = _screenshot_xwd(out_path)
    if img is not None:
        typer.echo(f"✓ Saved via xwd: {out_path}  [{img.width}×{img.height}]")
        if not no_analysis:
            _analyze_regions(img)
        return

    # Fallback: pyautogui screenshot
    typer.echo("⚠ xwd unavailable, falling back to pyautogui", err=True)
    pag = _pag()
    geo = _find_window()
    if geo:
        x, y, w, h = geo
        img = pag.screenshot(region=(x, y, w, h))
        typer.echo(f"✓ Captured window region ({x},{y}) {w}×{h}")
    else:
        typer.echo("⚠ Window not found; capturing full screen.", err=True)
        img = pag.screenshot()

    img.save(str(out_path))
    typer.echo(f"✓ Saved: {out_path}  [{img.width}×{img.height}]")

    if not no_analysis:
        _analyze_regions(img)


def _analyze_regions(img) -> None:  # img: PIL.Image.Image
    """Print mean RGB + brightness for 9 diagnostic regions."""
    _, ImageStat = _pil()
    w, h = img.size
    regions: dict[str, tuple[int, int, int, int]] = {
        "top-left": (0, 0, w // 3, h // 3),
        "top-center": (w // 3, 0, 2 * w // 3, h // 3),
        "top-right": (2 * w // 3, 0, w, h // 3),
        "mid-left": (0, h // 3, w // 3, 2 * h // 3),
        "center": (w // 3, h // 3, 2 * w // 3, 2 * h // 3),
        "mid-right": (2 * w // 3, h // 3, w, 2 * h // 3),
        "bot-left": (0, 2 * h // 3, w // 3, h),
        "bot-center": (w // 3, 2 * h // 3, 2 * w // 3, h),
        "bot-right": (2 * w // 3, 2 * h // 3, w, h),
        "full": (0, 0, w, h),
    }

    typer.echo("\n── Region Pixel Analysis ──────────────────────────────────")
    typer.echo(f"  {'region':<14}  {'R':>6} {'G':>6} {'B':>6}  {'brightness':>10}  label")
    typer.echo(f"  {'-' * 14}  {'-' * 6} {'-' * 6} {'-' * 6}  {'-' * 10}  -----")

    for name, box in regions.items():
        crop = img.crop(box)
        stat = ImageStat.Stat(crop)
        r, g, b = stat.mean[:3]
        brightness = (r + g + b) / 3.0
        if brightness < 20:
            label = "black/off"
        elif brightness < 60:
            label = "very dark"
        elif brightness < 120:
            label = "dark"
        elif brightness < 180:
            label = "mid"
        elif brightness < 230:
            label = "bright"
        else:
            label = "very bright"
        typer.echo(f"  {name:<14}  {r:6.1f} {g:6.1f} {b:6.1f}  {brightness:10.1f}  {label}")
    typer.echo("")


@cli.command(context_settings={"allow_extra_args": True, "ignore_unknown_options": True})
def orbit(
    args: Annotated[list[str], typer.Argument(help="dx dy — pixel deltas (may be negative).")],
    duration: float = typer.Option(0.3, "--duration", help="Drag animation duration (seconds)."),
) -> None:
    """
    Left-mouse drag to orbit the camera.

    Drag starts from the window center. Large values (±200-400 px) produce
    significant rotation. Camera pitch is clamped to ±1.25 rad (~±72°).

    Example: orbit 200 -50
    """
    if len(args) < 2:
        typer.echo("Error: provide dx and dy  e.g. orbit 200 -50", err=True)
        raise typer.Exit(1)
    try:
        dx, dy = int(args[0]), int(args[1])
    except ValueError:
        typer.echo("Error: dx and dy must be integers", err=True)
        raise typer.Exit(1)

    geo = _require_window()
    wx, wy, ww, wh = geo
    start_x = wx + ww // 2
    start_y = wy + wh // 2

    pag = _pag()
    pag.moveTo(start_x, start_y)
    time.sleep(0.05)
    pag.mouseDown(button="left")
    time.sleep(0.05)
    pag.moveTo(
        start_x + dx,
        start_y + dy,
        duration=duration,
        tween=_ease_in_out_quad,
    )
    time.sleep(0.05)
    pag.mouseUp(button="left")
    typer.echo(f"✓ Orbit drag ({dx:+d}, {dy:+d})  from ({start_x},{start_y})")


@cli.command(context_settings={"allow_extra_args": True, "ignore_unknown_options": True})
def zoom(
    args: Annotated[
        list[str],
        typer.Argument(help="steps — positive = zoom in, negative = zoom out."),
    ],
) -> None:
    """
    Scroll mouse wheel to zoom the camera.

    Each step moves distance by ~0.35 units (OrbitCamera::Zoom).
    Camera distance is clamped to [2, 18].

    Example: zoom 3   zoom -2
    """
    if not args:
        typer.echo("Error: provide steps  e.g. zoom 3  or zoom -2", err=True)
        raise typer.Exit(1)
    try:
        steps = float(args[0])
    except ValueError:
        typer.echo("Error: steps must be a number", err=True)
        raise typer.Exit(1)

    cx, cy = _window_center()
    pag = _pag()
    pag.moveTo(cx, cy)
    time.sleep(0.05)
    n = max(1, abs(int(steps)))
    direction = 1 if steps > 0 else -1
    for _ in range(n):
        pag.scroll(direction)
        time.sleep(0.04)
    typer.echo(f"✓ Scrolled {'in' if direction > 0 else 'out'} × {n}  at ({cx},{cy})")


@cli.command()
def key(
    k: str = typer.Argument(
        ...,
        help="Key name (pyautogui format): 'escape', 'r', 'space', 'f1', etc.",
    ),
) -> None:
    """Send a keyboard key to the app window (focuses window first via click)."""
    geo = _require_window()
    wx, wy, ww, wh = geo
    cx, cy = wx + ww // 2, wy + wh // 2
    pag = _pag()
    pag.click(cx, cy)
    time.sleep(0.05)
    pag.press(k)
    typer.echo(f"✓ Key pressed: {k!r}")


@cli.command()
def click(
    rel_x: float = typer.Argument(..., help="X as fraction of window width (0.0–1.0)."),
    rel_y: float = typer.Argument(..., help="Y as fraction of window height (0.0–1.0)."),
    button: str = typer.Option("left", help="Mouse button: left, right, middle."),
) -> None:
    """
    Click at a window-relative position (fractions, resolution-independent).

    Example: `click 0.5 0.5` clicks the exact window center.
    """
    geo = _require_window()
    wx, wy, ww, wh = geo
    abs_x = wx + int(rel_x * ww)
    abs_y = wy + int(rel_y * wh)
    _pag().click(abs_x, abs_y, button=button)
    typer.echo(f"✓ {button}-clicked ({rel_x:.2f},{rel_y:.2f}) → screen ({abs_x},{abs_y})")


@cli.command()
def info() -> None:
    """
    Full diagnostic dump: process, window geometry, controls, and screenshot analysis.

    Primary command for AI observation — run after any interaction to get a
    complete picture of the current render state.
    """
    typer.echo("═══ Future's Gaze Diagnostics ════════════════════════════\n")

    # ── Process ──
    if PID_FILE.exists():
        pid = int(PID_FILE.read_text().strip())
        try:
            os.kill(pid, 0)
            status = "running"
        except ProcessLookupError:
            status = "NOT running (PID stale)"
        typer.echo(f"Process  : PID={pid}  [{status}]")
    else:
        typer.echo("Process  : no PID file (not launched via this script)")

    typer.echo(f"Binary   : {BINARY}  [{'exists' if BINARY.exists() else 'MISSING'}]")
    typer.echo(f"DISPLAY  : {os.environ.get('DISPLAY', '(not set)')}")

    # ── Window ──
    _ensure_display()
    geo = _find_window()
    if geo:
        x, y, w, h = geo
        typer.echo(f"Window   : {w}×{h}  at screen ({x},{y})")
    else:
        typer.echo("Window   : NOT FOUND")

    # ── Controls reference ──
    typer.echo("\n── Controls ──────────────────────────────────────────────")
    typer.echo("  ESC             → quit")
    typer.echo("  Left-drag       → orbit camera (yaw + pitch)")
    typer.echo("  Scroll up/down  → zoom in/out (step ≈0.35, distance clamp 2–18)")
    typer.echo("  Camera pitch    : clamped ±1.25 rad (~±72°)")

    # ── Screenshot + analysis ──
    typer.echo("")
    screenshot()


# ── Entry point ────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    cli()
