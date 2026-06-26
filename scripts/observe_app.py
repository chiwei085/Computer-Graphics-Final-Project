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
  gaze        - toggle G mode, left-drag to rotate the Prediction Core
  gaze-future / gaze-longing / gaze-overlap
              - reset the Prediction Core aim, drag to a fixed story check pose,
                and save a screenshot
  zoom        - mouse-wheel scroll to zoom camera
  key         - send keyboard key to the app window
  click       - click at window-relative coordinates (fractions 0.0-1.0)
  scripted-gaze
              - launch with deterministic app-side gaze automation + stderr logs
  stderr-log  - print recent app stderr observation logs
  record      - deterministic observer-build framebuffer recording to MP4
  motion      - analyze deterministic MP4 + manifest motion metrics

Usage:
  uv run python observe_app.py info
  uv run python observe_app.py launch --build
  uv run python observe_app.py screenshot --out /tmp/frame.png
  uv run python observe_app.py orbit 200 -50
  uv run python observe_app.py gaze 180 40
  uv run python observe_app.py zoom 3
  uv run python observe_app.py scripted-gaze future --build
  uv run python observe_app.py stderr-log --tail 160
"""

from __future__ import annotations

import json
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
OBSERVER_BUILD_DIR = PROJECT_ROOT / "build" / "observer-debug"
OBSERVER_BINARY = OBSERVER_BUILD_DIR / "future_gaze"
FRAME_BRIDGE = OBSERVER_BUILD_DIR / "future_gaze_frame_bridge"
WINDOW_TITLE = "Future's Gaze"
PID_FILE = Path("/tmp/future_gaze.pid")
LOG_FILE = Path("/tmp/future_gaze.stderr.log")
SCREENSHOT_DIR = PROJECT_ROOT / "docs" / "screenshots"
STORY_POSES: dict[str, tuple[int, int] | None] = {
    "future": (0, 270),
    "longing": (0, 215),
    "overlap": (115, 235),
    "blindspot": (420, 0),
    "none": None,
}

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


def _timestamp_tag() -> str:
    """Return a collision-resistant timestamp for generated media filenames."""
    return time.strftime("%Y%m%d_%H%M%S") + f"_{time.time_ns() % 1_000_000_000:09d}"


def _build_dir_for_preset(preset: str) -> Path:
    return PROJECT_ROOT / "build" / preset


def _binary_for_preset(preset: str) -> Path:
    return _build_dir_for_preset(preset) / "future_gaze"


def _print_dynamic_observation_warning(command: str) -> None:
    typer.echo(
        f"✗ `{command}` is disabled by default: xwd burst capture can starve "
        "the GLUT loop and freeze app time under WSLg.",
        err=True,
    )
    typer.echo(
        "  Use deterministic app-side automation instead, e.g. "
        "`cd scripts && uv run python observe_app.py scripted-gaze future --build`.",
        err=True,
    )
    typer.echo(
        f"  Then inspect ground truth with `uv run python observe_app.py "
        f"stderr-log --tail 160` ({LOG_FILE}).",
        err=True,
    )


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
    "- preview" to WINDOW_TITLE.  First finds the window ID from the tree, then
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
def build(
    preset: str = typer.Option(
        "clang-debug",
        "--preset",
        help="CMake preset to configure/build.",
    ),
) -> None:
    """Configure and build the project with a CMake preset."""
    typer.echo("→ cmake configure...")
    subprocess.run(
        ["cmake", "--preset", preset],
        cwd=PROJECT_ROOT,
        check=True,
    )
    typer.echo("→ cmake build...")
    subprocess.run(
        ["cmake", "--build", "--preset", preset],
        cwd=PROJECT_ROOT,
        check=True,
    )
    typer.echo(f"✓ Build complete: {_binary_for_preset(preset)}")


@cli.command()
def launch(
    do_build: bool = typer.Option(False, "--build", "-b", help="Build before launching."),
    wait: float = typer.Option(2.0, "--wait", help="Seconds to wait for window after launch."),
    automation: Annotated[
        Path | None,
        typer.Option("--automation", help="Timed app-side command script to run in GLUT idle."),
    ] = None,
    log_every: float = typer.Option(
        0.0,
        "--log-every",
        help="Emit deterministic observation state every N seconds to stderr log.",
    ),
) -> None:
    """Launch the app in the background. PID saved to /tmp/future_gaze.pid."""
    if do_build:
        build(preset="clang-debug")

    if not BINARY.exists():
        typer.echo(f"✗ Binary not found: {BINARY}. Run `build` first.", err=True)
        raise typer.Exit(1)

    _ensure_display()
    env = os.environ.copy()
    if automation is not None:
        env["FUTURE_GAZE_AUTOMATION"] = str(automation)
    if log_every > 0.0:
        env["FUTURE_GAZE_LOG_EVERY"] = f"{log_every:.3f}"

    log = LOG_FILE.open("wb")
    proc = subprocess.Popen(
        [str(BINARY)],
        cwd=PROJECT_ROOT,
        env=env,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=log,
        start_new_session=True,
    )
    log.close()
    PID_FILE.write_text(str(proc.pid))
    typer.echo(f"✓ Launched PID={proc.pid}. Waiting {wait}s for window...")
    time.sleep(wait)

    geo = _find_window()
    if geo:
        x, y, w, h = geo
        typer.echo(f"✓ Window found: {w}×{h} at screen ({x},{y})")
    else:
        typer.echo("⚠ Window not found (may still be starting up)")
    typer.echo(f"stderr log: {LOG_FILE}")


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
    ts = _timestamp_tag()
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


def _drag_gaze(dx: int, dy: int, duration: float) -> None:
    typer.echo(
        "⚠ pyautogui input is focus-dependent under WSLg; for deterministic "
        "checks use `scripted-gaze`.",
        err=True,
    )
    geo = _require_window()
    wx, wy, ww, wh = geo
    start_x = wx + ww // 2
    start_y = wy + wh // 2

    pag = _pag()
    pag.click(start_x, start_y)
    time.sleep(0.05)
    pag.press("g")
    time.sleep(0.05)
    pag.moveTo(start_x, start_y)
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
    time.sleep(0.05)
    pag.press("g")
    typer.echo(f"✓ Gaze drag ({dx:+d}, {dy:+d})  from ({start_x},{start_y})")


def _story_pose(name: str, dx: int, dy: int, duration: float) -> None:
    typer.echo(f"→ story {name}: reset gaze, drag, then screenshot")
    geo = _require_window()
    wx, wy, ww, wh = geo
    pag = _pag()
    pag.click(wx + ww // 2, wy + wh // 2)
    time.sleep(0.05)
    pag.press("h")
    time.sleep(0.10)
    _drag_gaze(dx, dy, duration)
    time.sleep(0.45)
    out = SCREENSHOT_DIR / f"story_{name}_{_timestamp_tag()}.png"
    screenshot(out=out, no_analysis=False)


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

    typer.echo(
        "⚠ pyautogui drag is focus-dependent under WSLg; verify with screenshot/logs.",
        err=True,
    )
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
def gaze(
    args: Annotated[list[str], typer.Argument(help="dx dy — pixel deltas (may be negative).")],
    duration: float = typer.Option(0.3, "--duration", help="Drag animation duration (seconds)."),
) -> None:
    """
    Toggle Gaze mode, left-drag the Prediction Core, then return to camera mode.

    This assumes the app starts in camera mode. It is intentionally symmetric
    (press G before and after) so repeated visual checks leave normal camera
    orbit controls available.

    Example: gaze 180 40
    """
    if len(args) < 2:
        typer.echo("Error: provide dx and dy  e.g. gaze 180 40", err=True)
        raise typer.Exit(1)
    try:
        dx, dy = int(args[0]), int(args[1])
    except ValueError:
        typer.echo("Error: dx and dy must be integers", err=True)
        raise typer.Exit(1)

    _drag_gaze(dx, dy, duration)


@cli.command()
def gaze_future(
    duration: float = typer.Option(0.34, "--duration", help="Drag animation duration."),
) -> None:
    """Fixed check: look at the tabletop glass accident chain and capture."""
    _story_pose("future", 0, 270, duration)


@cli.command()
def gaze_longing(
    duration: float = typer.Option(0.34, "--duration", help="Drag animation duration."),
) -> None:
    """Fixed check: look toward the rear empty chairs and capture."""
    _story_pose("longing", 0, 215, duration)


@cli.command()
def gaze_overlap(
    duration: float = typer.Option(0.34, "--duration", help="Drag animation duration."),
) -> None:
    """Fixed check: hold the empty-chair overlap composition and capture."""
    _story_pose("overlap", 115, 235, duration)


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
    typer.echo(
        "⚠ pyautogui key delivery is focus-dependent under WSLg; use app-side "
        "automation for timing-sensitive checks.",
        err=True,
    )
    geo = _require_window()
    wx, wy, ww, wh = geo
    cx, cy = wx + ww // 2, wy + wh // 2
    pag = _pag()
    pag.click(cx, cy)
    time.sleep(0.05)
    pag.press(k)
    typer.echo(f"✓ Key pressed: {k!r}")


@cli.command("stderr-log")
def stderr_log(
    tail: int = typer.Option(120, "--tail", "-n", help="Number of recent log lines to print."),
) -> None:
    """Print recent stderr from the app launched by this script."""
    if not LOG_FILE.exists():
        typer.echo(f"✗ Log not found: {LOG_FILE}. Launch the app with this script first.", err=True)
        raise typer.Exit(1)
    for line in LOG_FILE.read_text(errors="replace").splitlines()[-tail:]:
        typer.echo(line)


def _terminate_existing_if_any() -> None:
    if not PID_FILE.exists():
        return
    try:
        pid = int(PID_FILE.read_text().strip())
    except ValueError:
        PID_FILE.unlink(missing_ok=True)
        return
    try:
        os.kill(pid, signal.SIGTERM)
        time.sleep(0.25)
    except ProcessLookupError:
        pass
    PID_FILE.unlink(missing_ok=True)


def _write_automation_script(pose: str) -> Path:
    if pose not in STORY_POSES:
        typer.echo(f"✗ Unknown pose {pose!r}. Choose: {', '.join(STORY_POSES)}", err=True)
        raise typer.Exit(1)

    script_path = Path("/tmp") / f"future_gaze_automation_{pose}_{_timestamp_tag()}.txt"
    commands = [
        "# seconds command args...",
        "0.05 log start",
    ]
    pose_delta = STORY_POSES[pose]
    if pose_delta is not None:
        dx, dy = pose_delta
        commands.extend(
            [
                "0.15 reset_gaze",
                "0.20 gaze_mode on",
                f"0.30 gaze_drag {dx} {dy}",
                "0.45 log aimed",
                "0.50 gaze_mode off",
                "0.65 log restage_start",
                "1.20 log restage_mid",
                "2.35 log settled",
            ]
        )
    else:
        commands.extend(
            [
                "0.50 log neutral",
                "1.20 log neutral_mid",
                "2.35 log neutral_settled",
            ]
        )
    commands.append("")
    script_path.write_text("\n".join(commands))
    return script_path


@cli.command("scripted-gaze")
def scripted_gaze(
    pose: str = typer.Argument("future", help="future, longing, overlap, or blindspot."),
    do_build: bool = typer.Option(False, "--build", "-b", help="Build before launching."),
    wait: float = typer.Option(2.0, "--wait", help="Seconds to wait for window after launch."),
    log_every: float = typer.Option(
        0.10,
        "--log-every",
        help="Per-frame-ish app-side state log interval in seconds.",
    ),
    kill_existing: bool = typer.Option(
        True,
        "--kill-existing/--keep-existing",
        help="Terminate a previous script-launched app before starting.",
    ),
) -> None:
    """Launch with deterministic app-side gaze automation and stderr state logs."""
    if pose not in STORY_POSES or STORY_POSES[pose] is None:
        allowed = [name for name, delta in STORY_POSES.items() if delta is not None]
        typer.echo(f"✗ Unknown pose {pose!r}. Choose: {', '.join(allowed)}", err=True)
        raise typer.Exit(1)

    if kill_existing:
        _terminate_existing_if_any()

    script_path = _write_automation_script(pose)
    typer.echo(f"✓ Automation script: {script_path}")
    launch(do_build=do_build, wait=wait, automation=script_path, log_every=log_every)


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
    typer.echo(f"Log file : {LOG_FILE}  [{'exists' if LOG_FILE.exists() else 'missing'}]")

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
    typer.echo("  G               → toggle Prediction Core gaze-drag mode")
    typer.echo("  H               → reset Prediction Core gaze aim")
    typer.echo("  V               → toggle gaze cone / blind-axis debug overlay")
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

    WARNING: intrusive under WSLg. Repeated xwd captures can starve the GLUT
    loop and make app time appear frozen. Keep this behind explicit
    --force-xwd gates; do not use it as animation timing ground truth.

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


def _ffmpeg_exe() -> str:
    try:
        import imageio_ffmpeg
    except ImportError:
        typer.echo(
            "✗ imageio-ffmpeg is not installed. Run `cd scripts && uv add imageio-ffmpeg`.",
            err=True,
        )
        raise typer.Exit(1)
    return imageio_ffmpeg.get_ffmpeg_exe()


def _load_manifest(path: Path) -> list[dict]:
    records: list[dict] = []
    for line in path.read_text().splitlines():
        if line.strip():
            records.append(json.loads(line))
    return records


def _read_video_grays(path: Path):
    import cv2
    import numpy as np

    cap = cv2.VideoCapture(str(path))
    if not cap.isOpened():
        typer.echo(f"✗ Cannot open video: {path}", err=True)
        raise typer.Exit(1)

    frames = []
    width = 0
    height = 0
    while True:
        ok, frame = cap.read()
        if not ok:
            break
        height, width = frame.shape[:2]
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY).astype(np.float32)
        frames.append(gray)
    cap.release()
    return frames, width, height


def _analyze_recorded_motion(video_path: Path, manifest_path: Path, flow: bool = True) -> None:
    import numpy as np

    records = _load_manifest(manifest_path)
    grays, w, h = _read_video_grays(video_path)
    n_manifest = len(records)
    n_video = len(grays)
    n = min(n_manifest, n_video)
    if n < 4:
        typer.echo(
            f"✗ Only {n} paired video/manifest frames — cannot analyze.",
            err=True,
        )
        raise typer.Exit(1)

    if n_video != n_manifest:
        typer.echo(
            f"⚠ video/manifest frame count mismatch: video={n_video}, manifest={n_manifest}; "
            f"analyzing first {n}",
            err=True,
        )
    grays = grays[:n]
    records = records[:n]

    frame_indices = np.array([int(r["frame_index"]) for r in records], dtype=np.int64)
    app_times = np.array([float(r["app_time"]) for r in records], dtype=np.float64)
    manifest_fps = float(records[0].get("fps", 0.0) or 0.0)
    if manifest_fps <= 0.0 and n > 1:
        manifest_fps = 1.0 / max(float(np.median(np.diff(app_times))), 1e-6)

    index_steps = np.diff(frame_indices)
    index_gaps = int(np.sum(np.maximum(index_steps - 1, 0))) if index_steps.size else 0
    non_monotonic = int(np.sum(index_steps != 1)) if index_steps.size else 0
    expected_dt = 1.0 / manifest_fps if manifest_fps > 0.0 else 0.0
    app_dt = np.diff(app_times)
    dt_error = (
        float(np.max(np.abs(app_dt - expected_dt))) if app_dt.size and expected_dt > 0.0 else 0.0
    )
    app_span = float(app_times[-1] - app_times[0]) if n > 1 else 0.0

    cv2 = None
    if flow:
        try:
            import cv2
        except ImportError:
            typer.echo(
                "⚠ opencv not installed; skipping flow. (uv add opencv-python-headless)",
                err=True,
            )
            flow = False

    dt = np.clip(app_dt, 1e-6, None)
    if flow and cv2 is not None:
        smalls = [cv2.resize(g.astype(np.uint8), (320, 180)) for g in grays]

    raw = np.array([np.mean(np.abs(grays[i] - grays[i - 1])) for i in range(1, n)])
    rate = raw / dt
    activity = float(raw.mean())
    activity_max = float(raw.max())
    median = float(np.median(raw)) if raw.size else 0.0
    frozen_frac = float(np.mean(raw < 0.02))
    moving_frac = float(np.mean(raw > 0.05))
    jerk = float(np.std(np.diff(rate)) / (rate.mean() + 1e-6))

    spikes = 0
    for i in range(1, len(raw) - 1):
        if raw[i] < 0.4 * median and raw[i - 1] > median and raw[i + 1] > median:
            spikes += 1

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
            vals = np.array(
                [
                    np.mean(np.abs(grays[i][y0:y1, x0:x1] - grays[i - 1][y0:y1, x0:x1]))
                    for i in range(1, n)
                ]
            )
            name = region_names[ry][rx]
            region_motion[name] = float(vals.mean())
            r_rate = vals / dt
            region_jerk[name] = float(np.std(np.diff(r_rate)) / (r_rate.mean() + 1e-6))

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
        path_smooth = float(max(0.0, 100.0 * (1.0 - min(norm_jerk / 35.0, 1.0))))
    else:
        path_smooth = 0.0

    vel_smooth = None
    flow_speed = None
    if flow and cv2 is not None:
        speeds = []
        prev = smalls[0]
        for i in range(1, len(smalls)):
            f = cv2.calcOpticalFlowFarneback(prev, smalls[i], None, 0.5, 3, 21, 3, 5, 1.2, 0)
            mag = np.sqrt(f[..., 0] ** 2 + f[..., 1] ** 2)
            moving_px = mag[mag > 0.25]
            speeds.append(float(moving_px.mean()) if moving_px.size else 0.0)
            prev = smalls[i]
        speeds = np.array(speeds)
        flow_speed = float(speeds.mean())
        if speeds.mean() > 1e-4:
            vc = np.std(np.diff(speeds)) / (speeds.mean() + 1e-6)
            vel_smooth = float(max(0.0, 100.0 * (1.0 - min(vc, 1.0))))
        else:
            vel_smooth = 0.0

    active = [name for name, m in region_motion.items() if m > 0.10]
    region_scores = [max(0.0, 100.0 * (1.0 - min(region_jerk[name] / 0.6, 1.0))) for name in active]
    region_smooth = float(np.mean(region_scores)) if region_scores else 0.0
    jerk_score = max(0.0, 100.0 * (1.0 - min(jerk / 0.6, 1.0)))
    spike_penalty = min(40.0, spikes * 6.0)
    smoothness = max(0.0, 0.5 * jerk_score + 0.5 * region_smooth - spike_penalty)
    if activity < 0.01:
        smoothness = 0.0

    typer.echo("\n══ Deterministic Motion Analysis ═════════════════════════")
    typer.echo(f"  video          : {video_path}")
    typer.echo(f"  manifest       : {manifest_path}")
    typer.echo(f"  frames         : {n_video} encoded, {n_manifest} manifest, {n} analyzed")
    typer.echo(f"  fps            : {manifest_fps:.3f} manifest fps")
    typer.echo(f"  app-time span  : {app_span:.6f}s  ({app_times[0]:.6f} → {app_times[-1]:.6f})")
    typer.echo(
        f"  frame checks   : gaps={index_gaps}  non-unit steps={non_monotonic}  "
        f"max dt error={dt_error:.9f}s"
    )
    typer.echo(f"  resolution     : {w}×{h}")
    typer.echo(f"  activity       : {activity:6.3f}   max {activity_max:6.3f}   (pixel Δ 0–255)")
    typer.echo(f"  moving frames  : {moving_frac * 100:5.1f}%   frozen {frozen_frac * 100:5.1f}%")
    typer.echo(f"  jerk           : {jerk:6.3f}   (lower = smoother; <0.30 is fluid)")
    typer.echo(f"  stutter spikes : {spikes}")
    typer.echo(f"  region smooth  : {region_smooth:5.1f} / 100")
    typer.echo(f"  path smooth    : {path_smooth:5.1f} / 100")
    if flow and flow_speed is not None and vel_smooth is not None:
        typer.echo(f"  flow speed     : {flow_speed:6.3f} px/frame")
        typer.echo(f"  velocity smooth: {vel_smooth:5.1f} / 100")

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
    typer.echo(f"  ▶ SMOOTHNESS   : {smoothness:5.1f} / 100   [{_grade(smoothness)}]")
    typer.echo("")


def _record_deterministic(
    pose: str,
    duration: float,
    fps: float,
    out: Path | None,
    manifest: Path | None,
    do_build: bool,
) -> tuple[Path, Path]:
    if pose not in STORY_POSES:
        typer.echo(f"✗ Unknown pose {pose!r}. Choose: {', '.join(STORY_POSES)}", err=True)
        raise typer.Exit(1)
    if duration <= 0.0:
        typer.echo("✗ --duration must be positive", err=True)
        raise typer.Exit(1)
    if fps <= 0.0:
        typer.echo("✗ --fps must be positive", err=True)
        raise typer.Exit(1)

    if do_build:
        build(preset="observer-debug")
    if not OBSERVER_BINARY.exists() or not FRAME_BRIDGE.exists():
        typer.echo(
            "✗ Observer tools are missing. Run with `--build` or build "
            "`cmake --preset observer-debug && cmake --build --preset observer-debug`.",
            err=True,
        )
        raise typer.Exit(1)

    SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
    tag = _timestamp_tag()
    out_path = out or (SCREENSHOT_DIR / f"clip_{pose}_{tag}.mp4")
    manifest_path = manifest or out_path.with_suffix(".jsonl")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)

    max_frames = max(1, round(duration * fps))
    fifo_path = Path("/tmp") / f"future_gaze_record_{tag}.fifo"
    fifo_path.unlink(missing_ok=True)
    os.mkfifo(fifo_path)
    automation_path = _write_automation_script(pose)
    app_log_path = Path("/tmp") / f"future_gaze_record_{tag}.stderr.log"

    _ensure_display()
    env = {
        **os.environ,
        "DISPLAY": os.environ.get("DISPLAY", ":0"),
        "FUTURE_GAZE_AUTOMATION": str(automation_path),
        "FUTURE_GAZE_LOG_EVERY": f"{1.0 / fps:.6f}",
        "FUTURE_GAZE_RECORD_FIFO": str(fifo_path),
        "FUTURE_GAZE_RECORD_FPS": f"{fps:.9f}",
        "FUTURE_GAZE_RECORD_MAX_FRAMES": str(max_frames),
        "FUTURE_GAZE_RECORD_FIXED_DT": "1",
    }

    ffmpeg = _ffmpeg_exe()
    typer.echo(
        f"→ recording {max_frames} frames ({duration:.3f}s @ {fps:.3f} fps) "
        f"pose={pose}"
    )
    typer.echo(f"→ automation: {automation_path}")

    bridge = None
    encoder = None
    app = None
    app_log = app_log_path.open("wb")
    try:
        bridge = subprocess.Popen(
            [str(FRAME_BRIDGE), "--fifo", str(fifo_path), "--manifest", str(manifest_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        encoder = subprocess.Popen(
            [
                ffmpeg,
                "-y",
                "-v",
                "error",
                "-f",
                "rawvideo",
                "-pix_fmt",
                "rgb24",
                "-s",
                "1280x720",
                "-r",
                f"{fps:.9f}",
                "-i",
                "pipe:0",
                "-pix_fmt",
                "yuv420p",
                str(out_path),
            ],
            stdin=bridge.stdout,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )
        if bridge.stdout is not None:
            bridge.stdout.close()
        app = subprocess.Popen(
            [str(OBSERVER_BINARY)],
            cwd=PROJECT_ROOT,
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=app_log,
            start_new_session=True,
        )

        timeout = max(30.0, duration * 12.0 + 15.0)
        app_rc = app.wait(timeout=timeout)
        bridge_rc = bridge.wait(timeout=15)
        _, encoder_err = encoder.communicate(timeout=15)
    except subprocess.TimeoutExpired:
        for proc in (app, bridge, encoder):
            if proc is not None and proc.poll() is None:
                proc.terminate()
        typer.echo("✗ Deterministic recording timed out.", err=True)
        raise typer.Exit(1)
    finally:
        app_log.close()
        fifo_path.unlink(missing_ok=True)

    bridge_err = bridge.stderr.read().decode(errors="replace") if bridge and bridge.stderr else ""
    encoder_rc = encoder.returncode if encoder is not None else 1
    encoder_err_text = encoder_err.decode(errors="replace") if encoder_err else ""
    if app_rc != 0:
        typer.echo(f"✗ App exited with rc={app_rc}. Log: {app_log_path}", err=True)
        raise typer.Exit(1)
    if bridge_rc != 0:
        typer.echo(f"✗ Bridge exited with rc={bridge_rc}: {bridge_err[-800:]}", err=True)
        raise typer.Exit(1)
    if encoder_rc != 0:
        typer.echo(f"✗ ffmpeg exited with rc={encoder_rc}: {encoder_err_text[-800:]}", err=True)
        raise typer.Exit(1)

    records = _load_manifest(manifest_path)
    typer.echo(f"✓ Saved: {out_path}")
    typer.echo(f"✓ Manifest: {manifest_path}  [{len(records)} frames]")
    typer.echo(f"stderr log: {app_log_path}")
    if len(records) != max_frames:
        typer.echo(
            f"⚠ expected {max_frames} manifest frames, got {len(records)}",
            err=True,
        )
    return out_path, manifest_path


@cli.command()
def record(
    out: Annotated[Path | None, typer.Option("--out", "-o", help="Output mp4 path.")] = None,
    duration: float = typer.Option(5.0, "--duration", "-d", help="Clip length (seconds)."),
    fps: float = typer.Option(30.0, "--fps", help="Deterministic recording FPS."),
    pose: str = typer.Option(
        "future",
        "--pose",
        help="Automation pose: future, longing, overlap, blindspot, none.",
    ),
    manifest: Annotated[
        Path | None,
        typer.Option("--manifest", help="Output JSONL frame manifest path."),
    ] = None,
    do_build: bool = typer.Option(
        False,
        "--build",
        "-b",
        help="Build observer-debug before recording.",
    ),
    analyze: bool = typer.Option(
        False,
        "--analyze",
        help="Analyze motion metrics from the encoded MP4 + manifest.",
    ),
    force_xwd: bool = typer.Option(
        False,
        "--force-xwd",
        help="Run the known-intrusive xwd burst capture anyway.",
    ),
) -> None:
    """
    Record a deterministic observer-build framebuffer stream to MP4.

    The default path records frames from inside the app render loop, flips them
    through the bridge helper, encodes with bundled ffmpeg, and writes a JSONL
    manifest. `--force-xwd` is retained only as an intrusive regression fallback.
    """
    import numpy as np

    if not force_xwd:
        out_path, manifest_path = _record_deterministic(
            pose=pose,
            duration=duration,
            fps=fps,
            out=out,
            manifest=manifest,
            do_build=do_build,
        )
        if analyze:
            _analyze_recorded_motion(out_path, manifest_path)
        return

    SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = out or (SCREENSHOT_DIR / f"clip_{_timestamp_tag()}_xwd_intrusive.mp4")
    typer.echo("⚠ forcing xwd burst capture; do not use this as timing ground truth.")
    typer.echo(f"→ capturing {duration:.1f}s via xwd ...")
    frames, ts, w, h = _capture_burst(duration, color=True)
    if len(frames) < 2:
        typer.echo("✗ Too few frames captured.", err=True)
        raise typer.Exit(1)
    eff_fps = len(frames) / (ts[-1] - ts[0]) if ts[-1] > ts[0] else 12.0
    enc = subprocess.run(
        [
            _ffmpeg_exe(),
            "-y",
            "-v",
            "error",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "bgr24",
            "-s",
            f"{w}x{h}",
            "-r",
            f"{eff_fps:.3f}",
            "-i",
            "pipe:0",
            "-pix_fmt",
            "yuv420p",
            str(out_path),
        ],
        input=np.concatenate([f.reshape(-1) for f in frames]).tobytes(),
        capture_output=True,
    )
    if enc.returncode != 0:
        typer.echo(enc.stderr[-400:].decode(errors="replace"), err=True)
        raise typer.Exit(1)
    typer.echo(f"✓ Saved: {out_path}  [{len(frames)} frames @ {eff_fps:.1f}fps]")


@cli.command()
def motion(
    video: Annotated[
        Path | None,
        typer.Option("--video", help="Existing MP4 to analyze."),
    ] = None,
    manifest: Annotated[
        Path | None,
        typer.Option(
            "--manifest",
            help="JSONL manifest for --video, or output path for recording.",
        ),
    ] = None,
    pose: str = typer.Option(
        "future",
        "--pose",
        help="Automation pose when recording: future, longing, overlap, blindspot, none.",
    ),
    duration: float = typer.Option(6.0, "--duration", "-d", help="Recording length (seconds)."),
    fps: float = typer.Option(30.0, "--fps", help="Deterministic recording FPS."),
    do_build: bool = typer.Option(
        False,
        "--build",
        "-b",
        help="Build observer-debug before recording.",
    ),
    flow: bool = typer.Option(True, "--flow/--no-flow", help="Run optical-flow velocity analysis."),
) -> None:
    """
    Analyze deterministic MP4 + manifest motion metrics.

    With --video and --manifest, analyzes an existing recording. Without
    --video, first records through the deterministic observer pipeline.
    """
    if video is not None:
        if manifest is None:
            typer.echo("✗ --manifest is required with --video", err=True)
            raise typer.Exit(1)
        _analyze_recorded_motion(video, manifest, flow=flow)
        return

    out_path, manifest_path = _record_deterministic(
        pose=pose,
        duration=duration,
        fps=fps,
        out=None,
        manifest=manifest,
        do_build=do_build,
    )
    _analyze_recorded_motion(out_path, manifest_path, flow=flow)


# ── Entry point ────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    cli()
