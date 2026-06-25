#!/usr/bin/env python3
"""Convert fetched GLB models to clean, normalized OBJ.

Handles KHR_draco_mesh_compression (NASA models use Draco).

Pipeline per model:
    load GLB -> decode Draco primitives -> apply scene-graph transforms
    -> recenter to origin -> scale to unit bounds -> export OBJ + MTL
"""

from __future__ import annotations

import struct
from pathlib import Path
from typing import Annotated

import numpy as np
import typer

REPO_ROOT = Path(__file__).resolve().parent.parent
RAW_DIR = REPO_ROOT / "assets" / "raw"
OBJ_DIR = REPO_ROOT / "assets" / "obj"

app = typer.Typer(help="Convert GLB assets to normalized OBJ.")


# ---------------------------------------------------------------------------
# GLB / glTF helpers
# ---------------------------------------------------------------------------

def _read_accessor(buf_data: bytes, gltf, acc_idx: int) -> np.ndarray | None:
    COMPONENT_TYPES = {
        5120: ("b", 1), 5121: ("B", 1), 5122: ("h", 2),
        5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4),
    }
    TYPE_COUNTS = {
        "SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4,
        "MAT2": 4, "MAT3": 9, "MAT4": 16,
    }
    acc = gltf.accessors[acc_idx]
    if acc.bufferView is None:
        return None
    bv = gltf.bufferViews[acc.bufferView]
    fmt_char, comp_size = COMPONENT_TYPES[acc.componentType]
    n_comps = TYPE_COUNTS[acc.type]
    stride = bv.byteStride or (comp_size * n_comps)
    offset = (bv.byteOffset or 0) + (acc.byteOffset or 0)
    values = [
        struct.unpack_from(f"{n_comps}{fmt_char}", buf_data, offset + i * stride)
        for i in range(acc.count)
    ]
    dtype = np.float32 if fmt_char == "f" else np.int32
    arr = np.array(values, dtype=dtype)
    return arr.reshape(acc.count, n_comps) if n_comps > 1 else arr.flatten()


def _node_transform(node) -> np.ndarray:
    if node.matrix:
        return np.array(node.matrix, dtype=np.float64).reshape(4, 4).T
    T = np.eye(4)
    if node.translation:
        T[0:3, 3] = node.translation
    R = np.eye(4)
    if node.rotation:
        qx, qy, qz, qw = node.rotation
        R[0, 0] = 1 - 2 * (qy * qy + qz * qz)
        R[0, 1] = 2 * (qx * qy - qz * qw)
        R[0, 2] = 2 * (qx * qz + qy * qw)
        R[1, 0] = 2 * (qx * qy + qz * qw)
        R[1, 1] = 1 - 2 * (qx * qx + qz * qz)
        R[1, 2] = 2 * (qy * qz - qx * qw)
        R[2, 0] = 2 * (qx * qz - qy * qw)
        R[2, 1] = 2 * (qy * qz + qx * qw)
        R[2, 2] = 1 - 2 * (qx * qx + qy * qy)
    S = np.eye(4)
    if node.scale:
        S[0, 0], S[1, 1], S[2, 2] = node.scale
    return T @ R @ S


def _collect_node_transforms(gltf) -> dict[int, np.ndarray]:
    n = len(gltf.nodes)
    parent: list[int] = [-1] * n
    for i, node in enumerate(gltf.nodes):
        for child in (node.children or []):
            parent[child] = i
    world: list[np.ndarray | None] = [None] * n

    def resolve(i: int) -> np.ndarray:
        if world[i] is not None:
            return world[i]  # type: ignore[return-value]
        local = _node_transform(gltf.nodes[i])
        world[i] = local if parent[i] == -1 else resolve(parent[i]) @ local
        return world[i]  # type: ignore[return-value]

    for i in range(n):
        resolve(i)
    return {i: world[i] for i in range(n) if world[i] is not None}


def _decode_primitive(
    prim, gltf, buf_data: bytes
) -> tuple[np.ndarray, np.ndarray, np.ndarray | None] | None:
    import DracoPy

    draco_ext = (prim.extensions or {}).get("KHR_draco_mesh_compression")
    if draco_ext:
        bv = gltf.bufferViews[draco_ext["bufferView"]]
        raw = buf_data[bv.byteOffset: bv.byteOffset + bv.byteLength]
        mesh = DracoPy.decode(raw)
        positions = np.array(mesh.points, dtype=np.float32)
        faces = np.array(mesh.faces, dtype=np.int32).reshape(-1, 3)
        normals = (np.array(mesh.normals, dtype=np.float32)
                   if mesh.normals is not None else None)
        return positions, faces, normals

    pos_idx = prim.attributes.POSITION
    if pos_idx is None:
        return None
    positions = _read_accessor(buf_data, gltf, pos_idx)
    if positions is None:
        return None
    idx = (_read_accessor(buf_data, gltf, prim.indices)
           if prim.indices is not None
           else np.arange(len(positions), dtype=np.int32))
    faces = idx.reshape(-1, 3)
    normals = (_read_accessor(buf_data, gltf, prim.attributes.NORMAL)
               if prim.attributes.NORMAL is not None else None)
    return positions, faces, normals


def _extract_material_color(
    mat,
) -> tuple[list[float], list[float], list[float], float]:
    kd, ka, ks, ns = [0.8, 0.8, 0.8], [0.1, 0.1, 0.1], [0.5, 0.5, 0.5], 32.0
    if mat is None:
        return ka, kd, ks, ns
    pbr = mat.pbrMetallicRoughness
    if pbr and pbr.baseColorFactor:
        kd = list(pbr.baseColorFactor[:3])
        ka = [x * 0.15 for x in kd]
        roughness = getattr(pbr, "roughnessFactor", 0.5) or 0.5
        ns = max(1.0, (1.0 - roughness) ** 2 * 128.0)
    spec_ext = (mat.extensions or {}).get("KHR_materials_specular", {})
    sf = spec_ext.get("specularColorFactor")
    if sf:
        ks = list(sf[:3])
    return ka, kd, ks, ns


# ---------------------------------------------------------------------------
# Core conversion
# ---------------------------------------------------------------------------

def _convert_one(src: Path, scale_to: float) -> Path | None:
    import pygltflib

    gltf = pygltflib.GLTF2().load(str(src))
    buf_data = bytes(gltf.binary_blob())
    node_transforms = _collect_node_transforms(gltf)

    all_positions: list[np.ndarray] = []
    all_normals: list[np.ndarray] = []
    all_faces: list[np.ndarray] = []
    mat_names: list[str] = []
    vertex_offset = 0

    for node_idx, node in enumerate(gltf.nodes):
        if node.mesh is None:
            continue
        world_mat = node_transforms.get(node_idx, np.eye(4))
        rot3 = world_mat[:3, :3]

        for prim in gltf.meshes[node.mesh].primitives:
            result = _decode_primitive(prim, gltf, buf_data)
            if result is None:
                continue
            positions, faces, normals = result

            ones = np.ones((len(positions), 1), dtype=np.float32)
            transformed = (world_mat @ np.hstack([positions, ones]).T).T[:, :3]
            all_positions.append(transformed.astype(np.float32))

            if normals is not None:
                all_normals.append((rot3 @ normals.T).T.astype(np.float32))

            all_faces.append(faces + vertex_offset)

            mat_name = f"mat_{len(mat_names)}"
            if prim.material is not None and prim.material < len(gltf.materials):
                raw = gltf.materials[prim.material].name or mat_name
                mat_name = raw.replace(" ", "_")
            mat_names.append(mat_name)
            vertex_offset += len(transformed)

    if not all_positions:
        typer.echo(f"[skip] {src.name}: no geometry decoded", err=True)
        return None

    positions = np.vstack(all_positions)
    centroid = (positions.min(axis=0) + positions.max(axis=0)) * 0.5
    extent = float(np.max(positions.max(axis=0) - positions.min(axis=0)))
    scale = scale_to / extent if extent > 0 else 1.0
    positions = (positions - centroid) * scale

    # Normals: use decoded if available, otherwise compute smooth per-vertex.
    if all_normals and len(all_normals) == len(all_positions):
        normals = np.vstack(all_normals)
        norms = np.linalg.norm(normals, axis=1, keepdims=True)
        norms[norms == 0] = 1.0
        normals /= norms
    else:
        faces_all = np.vstack(all_faces)
        normals = np.zeros_like(positions)
        v0 = positions[faces_all[:, 0]]
        v1 = positions[faces_all[:, 1]]
        v2 = positions[faces_all[:, 2]]
        fn = np.cross(v1 - v0, v2 - v0)
        ln = np.linalg.norm(fn, axis=1, keepdims=True)
        ln[ln == 0] = 1.0
        fn /= ln
        for fi, face in enumerate(faces_all):
            normals[face[0]] += fn[fi]
            normals[face[1]] += fn[fi]
            normals[face[2]] += fn[fi]
        ln2 = np.linalg.norm(normals, axis=1, keepdims=True)
        ln2[ln2 == 0] = 1.0
        normals /= ln2

    OBJ_DIR.mkdir(parents=True, exist_ok=True)
    stem = src.stem.replace(" ", "_")
    obj_path = OBJ_DIR / f"{stem}.obj"
    mtl_path = OBJ_DIR / f"{stem}.mtl"

    # Build MTL — deduplicate by name.
    mat_colors: dict[str, tuple] = {}
    for mi, mname in enumerate(mat_names):
        if mname in mat_colors:
            continue
        glb_mat = next(
            (m for m in (gltf.materials or [])
             if (m.name or "").replace(" ", "_") == mname or f"mat_{mi}" == mname),
            None,
        )
        mat_colors[mname] = _extract_material_color(glb_mat)

    with mtl_path.open("w") as mf:
        mf.write("# generated by scripts/convert_glb_to_obj.py\n\n")
        for mname, (ka, kd, ks, ns) in mat_colors.items():
            mf.write(f"newmtl {mname}\n")
            mf.write(f"Ka {ka[0]:.6f} {ka[1]:.6f} {ka[2]:.6f}\n")
            mf.write(f"Kd {kd[0]:.6f} {kd[1]:.6f} {kd[2]:.6f}\n")
            mf.write(f"Ks {ks[0]:.6f} {ks[1]:.6f} {ks[2]:.6f}\n")
            mf.write(f"Ns {ns:.2f}\n\n")

    faces_all = np.vstack(all_faces)
    with obj_path.open("w") as of:
        of.write(f"# {src.name} — converted by scripts/convert_glb_to_obj.py\n")
        of.write(f"mtllib {mtl_path.name}\n\n")
        for v in positions:
            of.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        of.write("\n")
        for n in normals:
            of.write(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
        of.write("\n")
        prev_mat = None
        for fi, faces_chunk in enumerate(all_faces):
            mname = mat_names[fi]
            if mname != prev_mat:
                of.write(f"\nusemtl {mname}\n")
                prev_mat = mname
            for face in faces_chunk:
                i0, i1, i2 = int(face[0]) + 1, int(face[1]) + 1, int(face[2]) + 1
                of.write(f"f {i0}//{i0} {i1}//{i1} {i2}//{i2}\n")

    total_verts = len(positions)
    total_faces = len(faces_all)
    typer.echo(
        f"[ok  ] {src.name} -> {obj_path.relative_to(REPO_ROOT)} "
        f"({total_verts} verts, {total_faces} tris, {len(mat_colors)} materials)"
    )
    return obj_path


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

@app.command()
def main(
    scale: Annotated[
        float,
        typer.Option(help="Target max bound (1.0 = unit cube)."),
    ] = 1.0,
    src: Annotated[
        list[Path] | None,
        typer.Argument(help="GLB files to convert (default: all in assets/raw/)."),
    ] = None,
) -> None:
    """Convert GLB files in assets/raw/ to normalized OBJ in assets/obj/."""
    if src:
        sources = src
    else:
        if not RAW_DIR.exists():
            typer.echo(
                "assets/raw/ not found — run: uv run python scripts/fetch_assets.py fetch",
                err=True,
            )
            raise typer.Exit(1)
        sources = sorted([*RAW_DIR.glob("*.glb"), *RAW_DIR.glob("*.gltf")])
        if not sources:
            typer.echo(f"no .glb/.gltf files in {RAW_DIR}", err=True)
            raise typer.Exit(1)

    converted = sum(
        1 for s in sources if _convert_one(s, scale) is not None
    )
    typer.echo(f"\nconverted {converted}/{len(sources)} model(s) -> assets/obj/")
    if converted < len(sources):
        raise typer.Exit(1)


if __name__ == "__main__":
    app()
