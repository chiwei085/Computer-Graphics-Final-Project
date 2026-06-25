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
  record      - capture the window to an mp4 via xwd frames + ffmpeg
  motion      - record a clip and report motion / smoothness / liveliness

Usage:
  uv run python observe_app.py info
  uv run python observe_app.py launch --build
  uv run python observe_app.py screenshot --out /tmp/frame.png
  uv run python observe_app.py orbit 200 -50
  uv run python observe_app.py zoom 3
  uv run python observe_app.py key escape
  uv run python observe_app.py motion --duration 5
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


# ── Motion capture & analysis ──────────────────────────────────────────────────


def _grab_frame(win_id: str, w: int, h: int, env: dict, color: bool):
    """Grab one window frame via xwd | ffmpeg → numpy array (gray or BGR).

    xwd reads the exact GL framebuffer by window id (works under WSLg, where
    ffmpeg x11grab only sees a black root). Returns None on a malformed frame.
    """
    import numpy as np

    pix = "bgr24" if color else "gray"
    xwd = subprocess.Popen(
        ["xwd", "-id", win_id, "-silent"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        env=env,
    )
    ff = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", "pipe:0", "-f", "rawvideo", "-pix_fmt", pix, "pipe:1"],
        stdin=xwd.stdout,
        capture_output=True,
    )
    xwd.stdout.close()  # type: ignore[union-attr]
    xwd.wait()
    data = ff.stdout
    expected = w * h * (3 if color else 1)
    if len(data) != expected:
        return None
    arr = np.frombuffer(data, np.uint8)
    return arr.reshape(h, w, 3) if color else arr.reshape(h, w)


def _capture_burst(duration: float, color: bool = False):
    """Capture window frames as fast as xwd allows for `duration` seconds.

    Returns (frames, timestamps, w, h). Real per-frame timestamps are returned
    so the analysis can normalize by actual dt (xwd cadence is ~10-15 fps and
    slightly uneven), keeping the smoothness metric honest.
    """
    import numpy as np

    _ensure_display()
    env = {**os.environ, "DISPLAY": os.environ.get("DISPLAY", ":0")}
    win_id = _find_window_id_xwininfo()
    if win_id is None:
        typer.echo("✗ Window not found. Is the app running?", err=True)
        raise typer.Exit(1)
    _, _, w, h = _require_window()

    frames: list = []
    ts: list[float] = []
    t0 = time.time()
    while time.time() - t0 < duration:
        f = _grab_frame(win_id, w, h, env, color)
        if f is not None:
            frames.append(f.astype(np.float32) if not color else f)
            ts.append(time.time())
    return frames, np.array(ts), w, h


def _grade(score: float) -> str:
    if score >= 85:
        return "excellent — fluid, high-class"
    if score >= 70:
        return "good — smooth"
    if score >= 55:
        return "acceptable — minor stutter"
    if score >= 35:
        return "rough — noticeable jerk/stutter"
    return "poor — stiff or broken"


@cli.command()
def record(
    out: Annotated[Path | None, typer.Option("--out", "-o", help="Output mp4 path.")] = None,
    duration: float = typer.Option(5.0, "--duration", "-d", help="Clip length (seconds)."),
) -> None:
    """
    Record the app window to an mp4 by capturing xwd frames (no input sent).

    Plays back at the real capture rate so the clip's timing matches what was
    seen. (ffmpeg x11grab cannot see the WSLg GL surface, so xwd is used.)
    """
    import numpy as np

    SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = out or (SCREENSHOT_DIR / f"clip_{int(time.time())}.mp4")
    typer.echo(f"→ capturing {duration:.1f}s via xwd ...")
    frames, ts, w, h = _capture_burst(duration, color=True)
    if len(frames) < 2:
        typer.echo("✗ Too few frames captured.", err=True)
        raise typer.Exit(1)
    eff_fps = len(frames) / (ts[-1] - ts[0]) if ts[-1] > ts[0] else 12.0
    enc = subprocess.run(
        ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "bgr24",
         "-s", f"{w}x{h}", "-r", f"{eff_fps:.3f}", "-i", "pipe:0",
         "-pix_fmt", "yuv420p", str(out_path)],
        input=np.concatenate([f.reshape(-1) for f in frames]).tobytes(),
        capture_output=True,
    )
    if enc.returncode != 0:
        typer.echo(enc.stderr[-400:].decode(errors="replace"), err=True)
        raise typer.Exit(1)
    typer.echo(f"✓ Saved: {out_path}  [{len(frames)} frames @ {eff_fps:.1f}fps]")


@cli.command()
def motion(
    duration: float = typer.Option(6.0, "--duration", "-d", help="Capture length (seconds)."),
    flow: bool = typer.Option(True, "--flow/--no-flow", help="Run optical-flow velocity analysis."),
) -> None:
    """
    Capture the self-running animation and report motion / smoothness metrics.

    Designed to judge whether the scene moves *and* moves smoothly ("流暢/高級感")
    rather than stiffly. Send no input first — the scene animates on its own.
    Frames are captured via xwd (WSLg-safe) at real timestamps; the motion
    signal is normalized to per-second rates so uneven capture cadence does not
    masquerade as jerk.

    Reported metrics:
      activity        inter-frame pixel change rate (is anything moving?)
      moving frames   fraction of frames with real motion (vs frozen)
      jerk            high-frequency variation of the motion rate (lower=smoother)
      velocity smooth optical-flow speed continuity 0-100 (higher=more fluid)
      per-region      where motion happens (3x3 grid) - verify each element moves
      SMOOTHNESS      overall 0-100 score + grade
    """
    import numpy as np

    typer.echo(f"→ capturing {duration:.1f}s via xwd (no input sent) ...")
    grays, ts, w, h = _capture_burst(duration, color=False)

    n = len(grays)
    if n < 4:
        typer.echo(f"✗ Only {n} frames captured — cannot analyze.", err=True)
        raise typer.Exit(1)

    cv2 = None
    if flow:
        try:
            import cv2
        except ImportError:
            typer.echo(
                "⚠ opencv not installed; skipping flow. "
                "(uv add opencv-python-headless)",
                err=True,
            )
            flow = False

    eff_fps = n / (ts[-1] - ts[0]) if ts[-1] > ts[0] else 0.0
    dt = np.diff(ts)  # real seconds between consecutive frames
    dt = np.clip(dt, 1e-3, None)

    # Downscaled copies for optical flow (cheaper, robust to texture noise).
    if flow:
        smalls = [cv2.resize(g, (320, 180)) for g in grays]

    # ── Inter-frame motion signal, normalized to per-second rate ──────────────
    raw = np.array([np.mean(np.abs(grays[i] - grays[i - 1])) for i in range(1, n)])
    rate = raw / dt  # pixel-Δ per second — independent of capture cadence
    activity = float(raw.mean())
    activity_max = float(raw.max())
    median = float(np.median(raw)) if raw.size else 0.0
    frozen_frac = float(np.mean(raw < 0.02))
    moving_frac = float(np.mean(raw > 0.05))

    # Jerk: abruptness of the (cadence-normalized) motion rate, normalized by its
    # own mean. Smooth animation → gradual change → low jerk.
    jerk = float(np.std(np.diff(rate)) / (rate.mean() + 1e-6))

    # Stutter spikes: low-motion frames sandwiched between higher-motion ones.
    spikes = 0
    for i in range(1, len(raw) - 1):
        if raw[i] < 0.4 * median and raw[i - 1] > median and raw[i + 1] > median:
            spikes += 1

    # ── Per-region localization (3x3) ─────────────────────────────────────────
    region_names = [
        ["top-left", "top-center", "top-right"],
        ["mid-left", "center", "mid-right"],
        ["bot-left", "bot-center", "bot-right"],
    ]
    region_motion: dict[str, float] = {}
    region_jerk: dict[str, float] = {}
    for ry in range(3):
        for rx in range(3):
            y0, y1 = ry * h // 3, (ry + 1) * h // 3
            x0, x1 = rx * w // 3, (rx + 1) * w // 3
            vals = np.array([
                np.mean(np.abs(grays[i][y0:y1, x0:x1] - grays[i - 1][y0:y1, x0:x1]))
                for i in range(1, n)
            ])
            region_motion[region_names[ry][rx]] = float(vals.mean())
            # Per-region temporal jerk — measures whether *this element's* motion
            # is smooth, free of the cross-mover confound that wrecks a global
            # centroid. Normalize the rate by dt so cadence jitter is removed.
            r_rate = vals / dt
            region_jerk[region_names[ry][rx]] = float(
                np.std(np.diff(r_rate)) / (r_rate.mean() + 1e-6)
            )

    # ── Centroid-trajectory smoothness ────────────────────────────────────────
    # Track where motion concentrates each frame (the motion centroid) and judge
    # how smoothly that point travels. A fluid orbit/turn traces a path whose
    # acceleration stays small relative to its velocity; stiff or snapping motion
    # spikes the acceleration. This is robust to sparse, small movers (unlike a
    # global optical-flow average) and needs no extra dependency.
    ys, xs = np.indices((h, w))
    xs = xs.astype(np.float32)
    ys = ys.astype(np.float32)
    cx = np.full(len(raw), w / 2.0, np.float32)
    cy = np.full(len(raw), h / 2.0, np.float32)
    for i in range(1, n):
        d = np.abs(grays[i] - grays[i - 1])
        s = float(d.sum())
        if s > 1.0:
            cx[i - 1] = float((xs * d).sum() / s)
            cy[i - 1] = float((ys * d).sum() / s)
        elif i > 1:
            cx[i - 1], cy[i - 1] = cx[i - 2], cy[i - 2]
    path = np.stack([cx, cy], axis=1)
    vel = np.diff(path, axis=0) / dt[1:, None]
    speed = np.linalg.norm(vel, axis=1)
    if len(vel) > 1 and speed.mean() > 1e-3:
        acc = np.diff(vel, axis=0) / dt[2:, None]
        norm_jerk = float(np.linalg.norm(acc, axis=1).mean() / (speed.mean() + 1e-6))
        # ~ <8 → fluid, >40 → snappy. Map to 0-100.
        path_smooth = float(max(0.0, 100.0 * (1.0 - min(norm_jerk / 35.0, 1.0))))
    else:
        norm_jerk = 0.0
        path_smooth = 0.0

    # ── Optical-flow velocity continuity (secondary, informational) ───────────
    vel_smooth = None
    flow_speed = None
    if flow:
        speeds = []
        prev = smalls[0]
        for i in range(1, len(smalls)):
            f = cv2.calcOpticalFlowFarneback(
                prev, smalls[i], None, 0.5, 3, 21, 3, 5, 1.2, 0
            )
            mag = np.sqrt(f[..., 0] ** 2 + f[..., 1] ** 2)
            # Average speed of the pixels that actually move (ignore still bg).
            moving_px = mag[mag > 0.25]
            speeds.append(float(moving_px.mean()) if moving_px.size else 0.0)
            prev = smalls[i]
        speeds = np.array(speeds)
        flow_speed = float(speeds.mean())
        if speeds.mean() > 1e-4:
            # Continuity = 1 - normalized step-to-step change of the speed curve.
            vc = np.std(np.diff(speeds)) / (speeds.mean() + 1e-6)
            vel_smooth = float(max(0.0, 100.0 * (1.0 - min(vc, 1.0))))
        else:
            vel_smooth = 0.0

    # ── Overall smoothness score ──────────────────────────────────────────────
    # Penalize jerk and stutter spikes; require that something is actually
    # moving (a frozen scene cannot be "smooth").
    # Average the per-region temporal smoothness over regions that actually move
    # (each animated element judged on its own), plus the global rate-jerk. The
    # centroid-path and optical-flow numbers are shown for context but excluded
    # from the score because multiple independent movers confound them.
    # Only score regions with substantive motion. Below ~0.10 the per-region
    # rate is dominated by sub-pixel aliasing of small bright/additive elements,
    # whose normalized jerk is noise rather than real stutter.
    active = [name for name, m in region_motion.items() if m > 0.10]
    region_scores = [
        max(0.0, 100.0 * (1.0 - min(region_jerk[name] / 0.6, 1.0))) for name in active
    ]
    region_smooth = float(np.mean(region_scores)) if region_scores else 0.0
    jerk_score = max(0.0, 100.0 * (1.0 - min(jerk / 0.6, 1.0)))
    spike_penalty = min(40.0, spikes * 6.0)
    smoothness = max(0.0, 0.5 * jerk_score + 0.5 * region_smooth - spike_penalty)
    if activity < 0.01:
        smoothness = 0.0  # nothing moved

    # ── Report ────────────────────────────────────────────────────────────────
    typer.echo("\n══ Motion / Smoothness Analysis ══════════════════════════")
    typer.echo(f"  frames analyzed : {n}  ({eff_fps:.1f} capture fps, {w}×{h})")
    typer.echo(f"  activity (mean) : {activity:6.3f}   max {activity_max:6.3f}   (pixel Δ 0–255)")
    typer.echo(f"  moving frames   : {moving_frac*100:5.1f}%   frozen {frozen_frac*100:5.1f}%")
    typer.echo(f"  jerk            : {jerk:6.3f}   (lower = smoother; <0.30 is fluid)")
    typer.echo(f"  stutter spikes  : {spikes}")
    typer.echo(
        f"  region smooth   : {region_smooth:5.1f} / 100   "
        "(per-element temporal jerk, primary)"
    )
    typer.echo(
        f"  path smoothness : {path_smooth:5.1f} / 100   "
        "(motion-centroid trajectory, ctx only)"
    )
    if flow:
        typer.echo(f"  flow speed      : {flow_speed:6.3f} px/frame")
        typer.echo(
            f"  velocity smooth : {vel_smooth:5.1f} / 100   "
            "(optical-flow continuity, ctx only)"
        )

    typer.echo("\n  ── per-region motion (rate, █) + temporal jerk (j) ──")
    hot = max(region_motion.values()) or 1.0
    for ry in range(3):
        cells = []
        for rx in range(3):
            name = region_names[ry][rx]
            v = region_motion[name]
            bar = "█" * round(6 * v / hot)
            jr = region_jerk[name] if v > 0.03 else 0.0
            cells.append(f"{v:4.2f}{bar:<6} j{jr:4.2f}")
        typer.echo("    " + " | ".join(cells))

    typer.echo("")
    typer.echo(f"  ▶ SMOOTHNESS    : {smoothness:5.1f} / 100   [{_grade(smoothness)}]")
    typer.echo("")


# ── Entry point ────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    cli()
