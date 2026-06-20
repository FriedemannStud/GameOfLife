#!/usr/bin/env python3
# KI-Agent unterstützt: Poster helper (F2.3) — generate print-ready SVG graphics
# from the live tournament data:
#   1) an aggregate win-rate-weighted occupancy heatmap (which board cells do
#      winners tend to occupy?), and
#   2) a row of the top-N start-pattern seed tiles.
# SVG is resolution-independent, so it scales cleanly to A0 poster size.
#
# Usage (host, after `docker-compose up`):
#     python3 backend/scripts/pattern_heatmap.py            # top 5, all 29 weighted
#     python3 backend/scripts/pattern_heatmap.py 8          # top 8 tiles
#
# Output: docs/Präsentation/260619_F2.3_heatmap.svg
#         docs/Präsentation/260619_F2.3_top_seeds.svg
#
# Reads MONGODB_URI / MONGODB_DB from the repo-root .env (see top_patterns.py).

import os
import sys

from dotenv import load_dotenv
from pymongo import MongoClient

GRID = 8


def cells_to_grid(cells):
    grid = [0] * (GRID * GRID)
    for cell in cells or []:
        if not cell or len(cell) < 2:
            continue
        x, y = int(cell[0]), int(cell[1])
        if 0 <= x < GRID and 0 <= y < GRID:
            grid[y * GRID + x] = 1
    return grid


def _connect():
    here = os.path.dirname(os.path.abspath(__file__))
    load_dotenv(os.path.join(here, "..", "..", ".env"))
    uri = os.getenv("TOP_PATTERNS_URI") or os.getenv("MONGODB_URI")
    if not uri:
        sys.exit("MONGODB_URI not set in repo-root .env")
    uri = uri.replace("@mongo:27017", "@127.0.0.1:27018")
    db_name = os.getenv("MONGODB_DB", "biotope_db")
    client = MongoClient(uri, serverSelectionTimeoutMS=4000)
    return client[db_name], db_name


def _heat_color(t):
    # t in [0,1] -> white (cold) to deep red (#b30000) ramp, poster-friendly
    r = int(255 + (179 - 255) * t)
    g = int(255 + (0 - 255) * t)
    b = int(255 + (0 - 255) * t)
    return f"#{r:02x}{g:02x}{b:02x}"


def build_heatmap_svg(heat, n_subs, dataset_label):
    cell = 60
    pad = 28
    label_h = 70
    legend_h = 60
    grid_px = GRID * cell
    w = grid_px + 2 * pad
    h = label_h + grid_px + legend_h + 2 * pad
    mx = max(heat) or 1.0

    s = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}" font-family="Helvetica,Arial,sans-serif">'
    ]
    s.append(f'<rect width="{w}" height="{h}" fill="white"/>')
    s.append(
        f'<text x="{w/2}" y="34" text-anchor="middle" font-size="26" '
        f'font-weight="bold">Wo gewinnen Muster?</text>'
    )
    s.append(
        f'<text x="{w/2}" y="60" text-anchor="middle" font-size="15" '
        f'fill="#555">Belegung der Felder, gewichtet nach Win-Rate '
        f"({n_subs} Einreichungen · {dataset_label})</text>"
    )

    y0 = label_h + pad
    for r in range(GRID):
        for c in range(GRID):
            t = heat[r * GRID + c] / mx
            x = pad + c * cell
            y = y0 + r * cell
            s.append(
                f'<rect x="{x}" y="{y}" width="{cell}" height="{cell}" '
                f'fill="{_heat_color(t)}" stroke="#ddd" stroke-width="1"/>'
            )

    # legend gradient bar
    ly = y0 + grid_px + 22
    s.append(
        '<defs><linearGradient id="g" x1="0" x2="1">'
        f'<stop offset="0" stop-color="{_heat_color(0)}"/>'
        f'<stop offset="1" stop-color="{_heat_color(1)}"/>'
        "</linearGradient></defs>"
    )
    s.append(
        f'<rect x="{pad}" y="{ly}" width="{grid_px}" height="18" '
        f'fill="url(#g)" stroke="#ccc"/>'
    )
    s.append(f'<text x="{pad}" y="{ly+38}" font-size="14" fill="#555">selten</text>')
    s.append(
        f'<text x="{pad+grid_px}" y="{ly+38}" text-anchor="end" '
        f'font-size="14" fill="#555">häufig bei Gewinnern</text>'
    )
    s.append("</svg>")
    return "\n".join(s)


def build_seeds_svg(tiles):
    cell = 26
    grid_px = GRID * cell
    tile_w = grid_px + 24
    tile_h = grid_px + 70
    cols = len(tiles)
    w = cols * tile_w + 24
    h = tile_h + 30

    s = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}" font-family="Helvetica,Arial,sans-serif">'
    ]
    s.append(f'<rect width="{w}" height="{h}" fill="white"/>')
    for i, (name, wr, grid) in enumerate(tiles):
        ox = 12 + i * tile_w
        oy = 40
        s.append(
            f'<text x="{ox+tile_w/2-12}" y="26" text-anchor="middle" '
            f'font-size="16" font-weight="bold">#{i+1}</text>'
        )
        for r in range(GRID):
            for c in range(GRID):
                fill = "#b30000" if grid[r * GRID + c] else "#f3f3f3"
                x = ox + c * cell
                y = oy + r * cell
                s.append(
                    f'<rect x="{x}" y="{y}" width="{cell}" height="{cell}" '
                    f'fill="{fill}" stroke="#ddd" stroke-width="0.6"/>'
                )
        ty = oy + grid_px + 22
        s.append(
            f'<text x="{ox+grid_px/2}" y="{ty}" text-anchor="middle" '
            f'font-size="15" font-weight="bold">{name}</text>'
        )
        s.append(
            f'<text x="{ox+grid_px/2}" y="{ty+20}" text-anchor="middle" '
            f'font-size="14" fill="#b30000">{wr}%</text>'
        )
    s.append("</svg>")
    return "\n".join(s)


def main():
    top_n = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    db, db_name = _connect()
    # Short tag for filenames/labels: biotope_db -> db, biotope_study -> study
    tag = db_name.replace("biotope_", "") or db_name
    query = {"status": "active", "matches_played": {"$gt": 0}}
    subs = list(
        db.submissions.find(query).sort(
            [("win_rate", -1), ("avg_stable_generation", 1)]
        )
    )
    if not subs:
        sys.exit("No ranked submissions found.")

    # Aggregate win-rate-weighted occupancy across ALL ranked submissions
    heat = [0.0] * (GRID * GRID)
    for sub in subs:
        wr = sub.get("win_rate", 0.0)
        grid = cells_to_grid(sub.get("config", {}).get("cells", []))
        for idx, v in enumerate(grid):
            if v:
                heat[idx] += wr

    tiles = []
    for sub in subs[:top_n]:
        name = sub.get("metadata", {}).get("nickname", "?")
        wr = round(sub.get("win_rate", 0.0) * 100, 1)
        tiles.append((name, wr, cells_to_grid(sub.get("config", {}).get("cells", []))))

    out_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "Präsentation"
    )
    heat_path = os.path.join(out_dir, f"260619_F2.3_heatmap_{tag}.svg")
    seeds_path = os.path.join(out_dir, f"260619_F2.3_top_seeds_{tag}.svg")
    with open(heat_path, "w") as f:
        f.write(build_heatmap_svg(heat, len(subs), db_name))
    with open(seeds_path, "w") as f:
        f.write(build_seeds_svg(tiles))

    print(f"Dataset: {db_name}")
    print(f"Wrote {os.path.normpath(heat_path)}")
    print(f"Wrote {os.path.normpath(seeds_path)}")
    print(f"(based on {len(subs)} ranked submissions, top {top_n} seed tiles)")


if __name__ == "__main__":
    main()
