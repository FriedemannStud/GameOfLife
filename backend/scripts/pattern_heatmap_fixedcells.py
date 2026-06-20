#!/usr/bin/env python3
# KI-Agent unterstützt: Poster helper (F2.3, follow-up) — "Wo gewinnen Muster?"
# restricted to start configurations with an EXACT live-cell count, so that
# density is held constant and only POSITION can vary. This isolates the
# question the aggregate heatmap could not answer: at equal density, does *where*
# a pattern sits decide the match?
#
# Produces three print-ready SVGs for a fixed cell count K (default 24):
#   1) a per-cell mean-win-rate heatmap (each cell coloured by the average
#      win-rate of the patterns that occupy it),
#   2) a winners-minus-losers occupancy-contrast heatmap (top vs bottom quartile),
#   3) a row of the top-N winning seed tiles among the K-cell patterns.
# It also runs a permutation (label-shuffle) test reporting whether the observed
# positional spread exceeds pure sampling noise.
#
# Usage (host, after `docker-compose up`):
#     MONGODB_DB=biotope_study python3 backend/scripts/pattern_heatmap_fixedcells.py
#     MONGODB_DB=biotope_study python3 \
#         backend/scripts/pattern_heatmap_fixedcells.py 24 5
#
# Reads MONGODB_URI / MONGODB_DB from the repo-root .env (see top_patterns.py).

import os
import random
import statistics
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


def _lerp_color(t, c0, c1):
    return "#" + "".join(f"{int(c0[i] + (c1[i] - c0[i]) * t):02x}" for i in range(3))


def _wr_color(t):
    # cool (blue, below mean) -> white (mean) -> warm (red, above mean)
    if t < 0.5:
        return _lerp_color(t / 0.5, (33, 102, 172), (247, 247, 247))
    return _lerp_color((t - 0.5) / 0.5, (247, 247, 247), (179, 0, 0))


def build_value_heatmap_svg(values, vmin, vmax, title, subtitle, lo_lbl, hi_lbl):
    cell = 60
    pad = 28
    label_h = 70
    legend_h = 64
    grid_px = GRID * cell
    w = grid_px + 2 * pad
    h = label_h + grid_px + legend_h + 2 * pad
    span = (vmax - vmin) or 1.0

    s = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}" font-family="Helvetica,Arial,sans-serif">'
    ]
    s.append(f'<rect width="{w}" height="{h}" fill="white"/>')
    s.append(
        f'<text x="{w/2}" y="34" text-anchor="middle" font-size="26" '
        f'font-weight="bold">{title}</text>'
    )
    s.append(
        f'<text x="{w/2}" y="60" text-anchor="middle" font-size="15" '
        f'fill="#555">{subtitle}</text>'
    )

    y0 = label_h + pad
    for r in range(GRID):
        for c in range(GRID):
            t = (values[r * GRID + c] - vmin) / span
            t = max(0.0, min(1.0, t))
            x = pad + c * cell
            y = y0 + r * cell
            s.append(
                f'<rect x="{x}" y="{y}" width="{cell}" height="{cell}" '
                f'fill="{_wr_color(t)}" stroke="#ddd" stroke-width="1"/>'
            )

    ly = y0 + grid_px + 22
    s.append(
        '<defs><linearGradient id="g" x1="0" x2="1">'
        f'<stop offset="0" stop-color="{_wr_color(0)}"/>'
        f'<stop offset="0.5" stop-color="{_wr_color(0.5)}"/>'
        f'<stop offset="1" stop-color="{_wr_color(1)}"/>'
        "</linearGradient></defs>"
    )
    s.append(
        f'<rect x="{pad}" y="{ly}" width="{grid_px}" height="18" '
        f'fill="url(#g)" stroke="#ccc"/>'
    )
    s.append(f'<text x="{pad}" y="{ly+38}" font-size="14" fill="#555">{lo_lbl}</text>')
    s.append(
        f'<text x="{pad+grid_px}" y="{ly+38}" text-anchor="end" '
        f'font-size="14" fill="#555">{hi_lbl}</text>'
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
    k = int(sys.argv[1]) if len(sys.argv) > 1 else 24
    top_n = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    db, db_name = _connect()
    tag = db_name.replace("biotope_", "") or db_name

    query = {"status": "active", "matches_played": {"$gt": 0}}
    subs = [
        s
        for s in db.submissions.find(query)
        if len(s.get("config", {}).get("cells", [])) == k
    ]
    if not subs:
        sys.exit(f"No ranked submissions with exactly {k} live cells.")
    n = len(subs)
    grids = [cells_to_grid(s.get("config", {}).get("cells", [])) for s in subs]
    wr = [s.get("win_rate", 0.0) for s in subs]
    gmean = statistics.mean(wr)

    # 1) per-cell mean win-rate among the patterns that occupy each cell
    occ = [0] * 64
    wsum = [0.0] * 64
    for grid, w in zip(grids, wr):
        for i, v in enumerate(grid):
            if v:
                occ[i] += 1
                wsum[i] += w
    mean_wr = [(wsum[i] / occ[i]) if occ[i] else gmean for i in range(64)]
    spread = statistics.pstdev([m * 100 for m in mean_wr])

    # 2) winners(top quartile) minus losers(bottom quartile) occupancy contrast
    order = sorted(range(n), key=lambda j: wr[j], reverse=True)
    qn = max(1, n // 4)
    top_idx, bot_idx = order[:qn], order[-qn:]
    contrast = [0] * 64
    for j in top_idx:
        for i, v in enumerate(grids[j]):
            if v:
                contrast[i] += 1
    for j in bot_idx:
        for i, v in enumerate(grids[j]):
            if v:
                contrast[i] -= 1
    cmax = max(abs(min(contrast)), abs(max(contrast))) or 1

    # 3) permutation test — is the positional spread above pure noise?
    def cell_spread(weights):
        ws = [0.0] * 64
        for grid, w in zip(grids, weights):
            for i, v in enumerate(grid):
                if v:
                    ws[i] += w
        return statistics.pstdev(
            [(ws[i] / occ[i] if occ[i] else gmean) * 100 for i in range(64)]
        )

    rng = random.Random(42)
    nulls = []
    shuffled = wr[:]
    for _ in range(500):
        rng.shuffle(shuffled)
        nulls.append(cell_spread(shuffled))
    null_mean = statistics.mean(nulls)
    pval = (sum(1 for x in nulls if x >= spread) + 1) / (len(nulls) + 1)

    # top seed tiles among the K-cell patterns
    tiles = []
    for j in order[:top_n]:
        s = subs[j]
        name = s.get("metadata", {}).get("nickname", f"seed-{s.get('_id')}")
        tiles.append((name, round(wr[j] * 100, 1), grids[j]))

    out_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "Präsentation"
    )
    heat_path = os.path.join(out_dir, f"260620_F2.3_heatmap_{k}cells_{tag}.svg")
    contrast_path = os.path.join(out_dir, f"260620_F2.3_contrast_{k}cells_{tag}.svg")
    seeds_path = os.path.join(out_dir, f"260620_F2.3_top_seeds_{k}cells_{tag}.svg")

    vmin, vmax = min(mean_wr) * 100, max(mean_wr) * 100
    with open(heat_path, "w") as f:
        f.write(
            build_value_heatmap_svg(
                [m * 100 for m in mean_wr],
                vmin,
                vmax,
                f"Wo gewinnen Muster? — bei exakt {k} Startzellen",
                f"Ø Win-Rate je Feld ({n} Muster · {db_name}) · "
                f"Spanne nur {vmin:.1f}–{vmax:.1f} %",
                f"{vmin:.1f} %",
                f"{vmax:.1f} %",
            )
        )
    with open(contrast_path, "w") as f:
        f.write(
            build_value_heatmap_svg(
                contrast,
                -cmax,
                cmax,
                f"Sitzen Gewinner woanders? — {k} Startzellen",
                f"Belegung Top- minus Bottom-Quartil (je {qn} Muster) · "
                f"Differenz max ±{cmax}",
                "Verlierer-Felder",
                "Gewinner-Felder",
            )
        )
    with open(seeds_path, "w") as f:
        f.write(build_seeds_svg(tiles))

    print(f"Dataset: {db_name}  |  exactly {k} live cells: {n} patterns")
    print(f"Global mean win-rate: {gmean*100:.2f} %")
    print(
        f"Per-cell mean-wr range: {vmin:.2f}–{vmax:.2f} %  "
        f"(positional spread {spread:.3f} pp)"
    )
    print(
        f"Permutation test: null spread {null_mean:.3f} pp, "
        f"observed {spread:.3f} pp, p = {pval:.3f}"
    )
    verdict = "ABOVE noise" if pval < 0.05 else "indistinguishable from noise"
    print(f"  -> positional signal is {verdict}.")
    print(f"Wrote {os.path.normpath(heat_path)}")
    print(f"Wrote {os.path.normpath(contrast_path)}")
    print(f"Wrote {os.path.normpath(seeds_path)}")


if __name__ == "__main__":
    main()
