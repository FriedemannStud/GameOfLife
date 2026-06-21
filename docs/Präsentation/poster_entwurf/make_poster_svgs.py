#!/usr/bin/env python3
# KI-Agent unterstützt: Generates the vector diagrams for the Biotop fair poster
# (draft in this directory). All grid/glider frames are computed with the real
# Conway step so they cannot drift from the game's actual behaviour. Output is
# A0-print-ready SVG (vector, scales losslessly). Re-run after any wording or
# colour change:
#     python3 make_poster_svgs.py
#
# Design follows the Uni-Bamberg guideline "Poster wirkungsvoll gestalten":
# large, simple, clear, consistent; sans-serif; strong colour contrast.

import os

HERE = os.path.dirname(os.path.abspath(__file__))

# --- consistent palette + typography (shared across all diagrams) -----------
RED = "#D7263D"  # Team Rot
BLUE = "#1B6CA8"  # Team Blau
ALIVE = "#2E3440"  # neutral living cell (rules / glider)
DEAD = "#EAEEF4"  # dead cell fill
GRIDLINE = "#AAB4C0"
INK = "#1A1A1A"  # body text
MUTE = "#5A6470"  # captions
ACCENT = "#0F8B6C"  # flow / "mitmachen" accent (green = go)
PAPER = "#FFFFFF"
FONT = "Arial, Tahoma, Helvetica, sans-serif"


def svg_header(w, h):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}" font-family="{FONT}">\n'
        f'<rect x="0" y="0" width="{w}" height="{h}" fill="{PAPER}"/>\n'
    )


def text(x, y, s, size, fill=INK, weight="normal", anchor="middle"):
    s = str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    return (
        f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" '
        f'font-weight="{weight}" text-anchor="{anchor}">{s}</text>\n'
    )


def grid(cells, n, x0, y0, cs, alive_color=ALIVE, focus=None, color_map=None):
    """n x n grid; `cells` = set of (x,y) alive; optional focus ring on a cell.
    `color_map` overrides the fill of individual living cells – used for the
    Biotop birth rule, where the team colours of the neighbours matter."""
    color_map = color_map or {}
    out = ""
    for gy in range(n):
        for gx in range(n):
            if (gx, gy) in cells:
                fill = color_map.get((gx, gy), alive_color)
            else:
                fill = DEAD
            out += (
                f'<rect x="{x0 + gx * cs}" y="{y0 + gy * cs}" width="{cs}" '
                f'height="{cs}" fill="{fill}" stroke="{GRIDLINE}" '
                f'stroke-width="2"/>\n'
            )
    if focus is not None:
        fx, fy = focus
        out += (
            f'<rect x="{x0 + fx * cs}" y="{y0 + fy * cs}" width="{cs}" '
            f'height="{cs}" fill="none" stroke="{ACCENT}" stroke-width="6"/>\n'
        )
    return out


def arrow(x1, y, x2, color=INK, w=6):
    head = 14
    return (
        f'<line x1="{x1}" y1="{y}" x2="{x2 - head}" y2="{y}" stroke="{color}" '
        f'stroke-width="{w}"/>\n'
        f'<polygon points="{x2},{y} {x2 - head},{y - head*0.7} '
        f'{x2 - head},{y + head*0.7}" fill="{color}"/>\n'
    )


def conway_step(cells, n):
    """One Conway step on an n x n board (dead outside, no wrap)."""
    nxt = set()
    for y in range(n):
        for x in range(n):
            live = sum(
                (nx, ny) in cells
                for nx in (x - 1, x, x + 1)
                for ny in (y - 1, y, y + 1)
                if not (nx == x and ny == y)
            )
            if ((x, y) in cells and live in (2, 3)) or (
                (x, y) not in cells and live == 3
            ):
                nxt.add((x, y))
    return nxt


# ---------------------------------------------------------------------------
# F2.1 – the four rules as before -> after icons
# ---------------------------------------------------------------------------
def make_vier_regeln():
    cs = 70
    # Panel must fully contain its content (2 grids + 80 px arrow channel),
    # otherwise neighbouring rules bleed into the gap and "stick" together.
    # The inter-rule gap (120) is kept clearly larger than the 80 px internal
    # arrow channel so the four rules read as four separate units (proximity).
    panel_w, gap, margin = 500, 120, 40
    n = 3
    w = panel_w * 4 + gap * 3 + margin * 2
    # Extra bottom room for the second caption line of the Biotop birth rule.
    h = 430
    s = svg_header(w, h)
    s += text(w / 2, 58, "Vier einfache Regeln – komplexes Verhalten", 40, INK, "bold")

    # The first three rules only count neighbours (colour is irrelevant), so
    # they stay neutral grey = classic Conway. The fourth rule is where Biotop
    # diverges: a newborn cell inherits the MAJORITY colour of its 3 neighbours
    # (3 is odd → never a tie). We show that with team colours (2 red + 1 blue
    # → born red) and an accent-green note marking it as the Biotop extension.
    # `extra` carries the per-cell colours and the second caption line.
    birth = {
        "before_colors": {(0, 1): RED, (2, 1): RED, (1, 0): BLUE},
        "after_colors": {(1, 1): RED},
        "cap2": "Biotop: erbt die Mehrheitsfarbe",
    }
    rules = [
        # (title, before-alive set, center-alive?, outcome-alive?, caption, extra)
        ("Überleben", {(0, 1), (2, 1)}, True, True, "lebt · 2 Nachbarn", None),
        ("Einsamkeit", {(0, 1)}, True, False, "stirbt · < 2", None),
        (
            "Überbevölkerung",
            {(0, 1), (2, 1), (1, 0), (1, 2)},
            True,
            False,
            "stirbt · > 3",
            None,
        ),
        ("Geburt", {(0, 1), (2, 1), (1, 0)}, False, True, "wird geboren · 3", birth),
    ]
    gx = margin
    for title, nb, c_before, c_after, cap, extra in rules:
        cx = gx + panel_w / 2
        # Label and caption hug the grid tightly so each rule reads as one
        # vertical block, clearly detached from the figure title above.
        s += text(cx, 118, title, 32, INK, "bold")
        gw = n * cs
        bx = gx + (panel_w - (gw * 2 + 80)) / 2
        by = 140
        before = set(nb) | ({(1, 1)} if c_before else set())
        after = {(1, 1)} if c_after else set()
        before_cm = extra["before_colors"] if extra else None
        after_cm = extra["after_colors"] if extra else None
        s += grid(before, n, bx, by, cs, focus=(1, 1), color_map=before_cm)
        s += arrow(bx + gw + 14, by + gw / 2, bx + gw + 66)
        s += grid(after, n, bx + gw + 80, by, cs, focus=(1, 1), color_map=after_cm)
        s += text(cx, by + gw + 38, cap, 26, MUTE)
        if extra and extra.get("cap2"):
            s += text(cx, by + gw + 64, extra["cap2"], 23, ACCENT, "bold")
        gx += panel_w + gap
    s += "</svg>\n"
    return "f21_vier_regeln.svg", s


# ---------------------------------------------------------------------------
# F2.1 – glider walking across the board (5 generations)
# ---------------------------------------------------------------------------
def make_gleiter():
    n = 7
    cs = 46
    gw = n * cs
    frames = 5
    gap = 70
    w = gw * frames + gap * (frames - 1) + 60
    h = gw + 150
    s = svg_header(w, h)
    s += text(
        w / 2, 56, "Der Gleiter – niemand hat ihm das Laufen befohlen", 40, INK, "bold"
    )
    # classic glider, top-left
    cells = {(1, 0), (2, 1), (0, 2), (1, 2), (2, 2)}
    x0 = 30
    y0 = 90
    for g in range(frames):
        s += grid(cells, n, x0, y0, cs)
        s += text(x0 + gw / 2, y0 + gw + 44, f"Generation {g}", 26, MUTE)
        if g < frames - 1:
            ax = x0 + gw + 8
            s += arrow(ax, y0 + gw / 2, ax + gap - 16, ACCENT, 6)
        cells = conway_step(cells, n)
        x0 += gw + gap
    s += "</svg>\n"
    return "f21_gleiter_sequenz.svg", s


# ---------------------------------------------------------------------------
# F3.2 – the 4-step "become a player" flow + simplified pipeline
# ---------------------------------------------------------------------------
def make_mitmach_ablauf():
    w, h = 1900, 560
    s = svg_header(w, h)
    s += text(w / 2, 60, "In 30 Sekunden vom Zuschauer zum Mitspieler", 40, INK, "bold")
    steps = [
        ("1 · MALEN", "8×8-Muster zeichnen", "Team Rot oder Blau", RED),
        ("2 · ABSENDEN", "Muster geht an den", "Server", BLUE),
        ("3 · KÄMPFEN", "tritt gegen alle", "anderen an", ALIVE),
        ("4 · RANKEN", "Name erscheint im", "Leaderboard", ACCENT),
    ]
    bw, bh = 380, 200
    gap = (w - 60 - bw * 4) / 3
    x = 30
    y = 110
    for i, (t, l1, l2, col) in enumerate(steps):
        s += (
            f'<rect x="{x}" y="{y}" width="{bw}" height="{bh}" rx="18" '
            f'fill="{PAPER}" stroke="{col}" stroke-width="6"/>\n'
        )
        s += text(x + bw / 2, y + 62, t, 36, col, "bold")
        s += text(x + bw / 2, y + 116, l1, 28, INK)
        s += text(x + bw / 2, y + 154, l2, 28, INK)
        if i < 3:
            ax = x + bw + 6
            s += arrow(ax, y + bh / 2, ax + gap - 12, MUTE, 7)
        x += bw + gap
    # simplified pipeline underneath
    py = y + bh + 90
    s += text(w / 2, py - 18, "Was im Hintergrund passiert (vereinfacht):", 26, MUTE)
    chain = ["Editor", "Backend", "Matchmaker", "Hyper-Worker", "Leaderboard"]
    cw = 300
    cgap = (w - 60 - cw * len(chain)) / (len(chain) - 1)
    cx = 30
    for i, label in enumerate(chain):
        s += (
            f'<rect x="{cx}" y="{py + 6}" width="{cw}" height="{72}" rx="12" '
            f'fill="{DEAD}" stroke="{GRIDLINE}" stroke-width="3"/>\n'
        )
        s += text(cx + cw / 2, py + 52, label, 28, INK, "bold")
        if i < len(chain) - 1:
            ax = cx + cw + 4
            s += arrow(ax, py + 42, ax + cgap - 8, INK, 5)
        cx += cw + cgap
    s += "</svg>\n"
    return "f32_mitmach_ablauf.svg", s


# ---------------------------------------------------------------------------
# F1.1 – three-layer architecture (laypeople-friendly)
# ---------------------------------------------------------------------------
def make_architektur():
    w, h = 1300, 760
    s = svg_header(w, h)
    s += text(w / 2, 58, "Ein verteiltes System aus drei Schichten", 40, INK, "bold")
    layers = [
        ("MITMACHEN  ·  Web", ["Editor", "Kiosk"], BLUE),
        ("SERVER  ·  Python", ["FastAPI", "Matchmaker-Worker", "MongoDB"], ACCENT),
        ("RECHENKERN  ·  C", ["GUI", "Headless", "Hyper-Worker"], RED),
    ]
    lw = w - 120
    lx = 60
    ly = 110
    lh = 170
    vgap = 50
    for title, boxes, col in layers:
        s += (
            f'<rect x="{lx}" y="{ly}" width="{lw}" height="{lh}" rx="20" '
            f'fill="{PAPER}" stroke="{col}" stroke-width="6"/>\n'
        )
        s += text(lx + 28, ly + 52, title, 32, col, "bold", anchor="start")
        n = len(boxes)
        inner_gap = 36
        ibw = (lw - 56 - inner_gap * (n - 1)) / n
        ix = lx + 28
        for b in boxes:
            s += (
                f'<rect x="{ix}" y="{ly + 78}" width="{ibw}" height="{66}" '
                f'rx="12" fill="{DEAD}" stroke="{GRIDLINE}" stroke-width="3"/>\n'
            )
            s += text(ix + ibw / 2, ly + 120, b, 28, INK, "bold")
            ix += ibw + inner_gap
        # connector arrow to next layer
        if title.startswith("MITMACHEN") or title.startswith("SERVER"):
            mid = lx + lw / 2
            s += (
                f'<line x1="{mid}" y1="{ly + lh}" x2="{mid}" y2="{ly + lh + vgap}" '
                f'stroke="{MUTE}" stroke-width="6"/>\n'
                f'<polygon points="{mid},{ly + lh + vgap} '
                f"{mid - 12},{ly + lh + vgap - 16} {mid + 12},"
                f'{ly + lh + vgap - 16}" fill="{MUTE}"/>\n'
            )
        ly += lh + vgap
    s += text(
        w / 2,
        h - 24,
        "Mehrere Programme laufen gleichzeitig in Containern (Docker) "
        "und reden übers Netz.",
        26,
        MUTE,
    )
    s += "</svg>\n"
    return "f11_architektur_3schichten.svg", s


# ---------------------------------------------------------------------------
# F1.1 – key figures bar  (counts recounted from the repo on 2026-06-20)
# ---------------------------------------------------------------------------
def make_kennzahlen():
    w, h = 1300, 300
    s = svg_header(w, h)
    s += text(
        w / 2, 56, "Gebaut im 1. Semester – Mensch + KI-Agent als Team", 36, INK, "bold"
    )
    chips = [
        ("≈ 13.300", "Zeilen eigener Code"),
        ("3", "native C-Programme"),
        ("17", "automatisierte Tests"),
        ("35", "ADRs (Architektur-\nEntscheidungen)"),
    ]
    cw = 280
    gap = (w - 80 - cw * len(chips)) / (len(chips) - 1)
    x = 40
    y = 96
    ch = 130
    for big, label in chips:
        s += (
            f'<rect x="{x}" y="{y}" width="{cw}" height="{ch}" rx="18" '
            f'fill="{DEAD}" stroke="{GRIDLINE}" stroke-width="3"/>\n'
        )
        s += text(x + cw / 2, y + 70, big, 54, RED, "bold")
        lines = label.split("\n")
        ly = y + 104
        for ln in lines:
            s += text(x + cw / 2, ly, ln, 24, INK)
            ly += 28
        x += cw + gap
    s += text(
        w / 2,
        h - 30,
        "C ≈ 4.400 · Python ≈ 3.100 · Web ≈ 5.800  "
        "(eingebettete cJSON-Bibliothek ~3.500 Zeilen nicht mitgezählt)",
        22,
        MUTE,
    )
    s += "</svg>\n"
    return "f11_kennzahlen.svg", s


def main():
    for fn in (
        make_vier_regeln,
        make_gleiter,
        make_mitmach_ablauf,
        make_architektur,
        make_kennzahlen,
    ):
        name, content = fn()
        with open(os.path.join(HERE, name), "w") as f:
            f.write(content)
        print(f"wrote {name}")


if __name__ == "__main__":
    main()
