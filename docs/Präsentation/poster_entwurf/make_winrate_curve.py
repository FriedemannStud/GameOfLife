#!/usr/bin/env python3
# KI-Agent unterstützt: Builds the "win-rate vs. cell count" curve for the poster
# from the FULL-range uniform study (biotope_study_uniform, cell_count 2..64).
# Unlike the older study (capped at 24 cells, which only saw the rising edge),
# this dataset reveals the complete inverted-U: a density sweet spot around
# 24-32 cells. Output: f23_winrate_vs_cells.svg (A0-print-ready vector).
#
# Re-run after a fresh tournament epoch:
#     python3 make_winrate_curve.py
#
# Reads MONGODB_URI from repo-root .env (rewrites docker host -> 27018).
# Forces the study DB regardless of MONGODB_DB so it can't read the wrong set.

import os
import sys

from dotenv import load_dotenv
from pymongo import MongoClient

HERE = os.path.dirname(os.path.abspath(__file__))
DB_NAME = "biotope_study_uniform"
OUT = os.path.join(HERE, "f23_winrate_vs_cells.svg")

# palette consistent with make_poster_svgs.py
RED = "#D7263D"
GO = "#0F8B6C"
INK = "#1A1A1A"
MUTE = "#5A6470"
LINE = "#AAB4C0"
BAND = "#0F8B6C"
FONT = "Arial, Tahoma, Helvetica, sans-serif"


def connect():
    load_dotenv(os.path.join(HERE, "..", "..", "..", ".env"))
    uri = os.getenv("TOP_PATTERNS_URI") or os.getenv("MONGODB_URI")
    if not uri:
        sys.exit("MONGODB_URI not set in repo-root .env")
    uri = uri.replace("@mongo:27017", "@127.0.0.1:27018")
    return MongoClient(uri, serverSelectionTimeoutMS=4000)[DB_NAME]


def fetch():
    col = connect().submissions
    pipe = [
        {"$match": {"win_rate": {"$exists": True}}},
        {
            "$group": {
                "_id": "$config.cell_count",
                "wr": {"$avg": "$win_rate"},
                "n": {"$sum": 1},
            }
        },
        {"$sort": {"_id": 1}},
    ]
    return [(r["_id"], r["wr"] * 100.0, r["n"]) for r in col.aggregate(pipe)]


def main():
    data = fetch()
    if not data:
        sys.exit(f"No ranked submissions in '{DB_NAME}' — run the tournament first.")
    n_total = sum(n for _, _, n in data)
    peak_c, peak_w, _ = max(data, key=lambda r: r[1])

    # --- canvas + plot box ---
    W, H = 1300, 780
    L, R, T, B = 130, 60, 150, 130
    pw, ph = W - L - R, H - T - B
    cmin, cmax = 2, 64
    ymax = 80.0

    def px(c):
        return L + (c - cmin) / (cmax - cmin) * pw

    def py(v):
        return T + (1 - v / ymax) * ph

    s = (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
        f'viewBox="0 0 {W} {H}" font-family="{FONT}">\n'
        f'<rect width="{W}" height="{H}" fill="#FFFFFF"/>\n'
    )
    # title + subtitle
    s += (
        f'<text x="{W/2}" y="56" text-anchor="middle" font-size="36" '
        f'font-weight="bold" fill="{INK}">Dichte-Optimum: nicht '
        f"„je voller, desto besser&quot;</text>\n"
    )
    s += (
        f'<text x="{W/2}" y="98" text-anchor="middle" font-size="23" '
        f'fill="{MUTE}">{n_total:,} Zufallsmuster · gleichverteilt 2–64 '
        f"Zellen · ~100 Mio. Duelle (Round-Robin)</text>\n".replace(",", ".")
    )

    # sweet-spot band 24..32
    bx, bw = px(24), px(32) - px(24)
    bmid = (px(24) + px(32)) / 2
    s += (
        f'<rect x="{bx:.1f}" y="{T}" width="{bw:.1f}" height="{ph}" '
        f'fill="{BAND}" opacity="0.10"/>\n'
    )
    s += (
        f'<text x="{bmid:.1f}" y="{T+32}" text-anchor="middle" '
        f'font-size="26" font-weight="bold" fill="{GO}">Sweet Spot</text>\n'
    )

    # y gridlines
    for v in (0, 20, 40, 60, 80):
        yy = py(v)
        s += (
            f'<line x1="{L}" y1="{yy:.1f}" x2="{W-R}" y2="{yy:.1f}" '
            f'stroke="#EEE"/>\n'
        )
        s += (
            f'<text x="{L-14}" y="{yy+8:.1f}" text-anchor="end" '
            f'font-size="24" fill="{MUTE}">{v} %</text>\n'
        )
    # 50% reference
    s += (
        f'<line x1="{L}" y1="{py(50):.1f}" x2="{W-R}" y2="{py(50):.1f}" '
        f'stroke="{LINE}" stroke-dasharray="6 5"/>\n'
    )
    s += (
        f'<text x="{W-R}" y="{py(50)-10:.1f}" text-anchor="end" '
        f'font-size="20" fill="{MUTE}">50 %</text>\n'
    )

    # x ticks
    for c in (2, 8, 16, 24, 32, 40, 48, 56, 64):
        s += (
            f'<text x="{px(c):.1f}" y="{H-B+44}" text-anchor="middle" '
            f'font-size="24" fill="{MUTE}">{c}</text>\n'
        )
    s += (
        f'<text x="{L+pw/2:.1f}" y="{H-B+86}" text-anchor="middle" '
        f'font-size="26" font-weight="bold" fill="{INK}">'
        f"lebende Zellen im Startmuster (von 64)</text>\n"
    )

    # the curve (polyline) + points
    pts = " ".join(f"{px(c):.1f},{py(w):.1f}" for c, w, _ in data)
    s += (
        f'<polyline points="{pts}" fill="none" stroke="{RED}" '
        f'stroke-width="6" stroke-linejoin="round"/>\n'
    )
    for c, w, _ in data:
        s += f'<circle cx="{px(c):.1f}" cy="{py(w):.1f}" r="4" fill="{RED}"/>\n'

    # peak marker + label (placed BELOW the marker to avoid the band caption)
    s += (
        f'<circle cx="{px(peak_c):.1f}" cy="{py(peak_w):.1f}" r="11" '
        f'fill="none" stroke="{GO}" stroke-width="5"/>\n'
    )
    s += (
        f'<text x="{bmid:.1f}" y="{py(peak_w)+52:.1f}" '
        f'text-anchor="middle" font-size="25" font-weight="bold" '
        f'fill="{GO}">Optimum ≈ {peak_w:.0f} % bei 24–32 Zellen</text>\n'
    )
    s += (
        f'<text x="{bmid:.1f}" y="{py(peak_w)+82:.1f}" '
        f'text-anchor="middle" font-size="22" '
        f'fill="{GO}">~⅓–½ des Bretts belegt</text>\n'
    )

    # end annotations
    s += (
        f'<text x="{px(4)+6:.1f}" y="{py(15):.1f}" text-anchor="start" '
        f'font-size="24" fill="{MUTE}">zu leer → verliert (~19 %)</text>\n'
    )
    s += (
        f'<text x="{px(62):.1f}" y="{py(46):.1f}" text-anchor="end" '
        f'font-size="24" fill="{MUTE}">zu voll → verliert (~40 %)</text>\n'
    )

    s += "</svg>\n"
    with open(OUT, "w") as f:
        f.write(s)
    print(
        f"wrote {os.path.relpath(OUT)}  (peak {peak_w:.1f}% @ {peak_c} cells, "
        f"n={n_total})"
    )


if __name__ == "__main__":
    main()
