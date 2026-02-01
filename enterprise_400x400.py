import math

def get_lwss_pattern():
    """
    Lightweight spaceship (LWSS), period 4.
    Standard LWSS:
    .#..#
    #....
    #...#
    ####.
    """
    return [
        (0, 1), (0, 4),
        (1, 0),
        (2, 0), (2, 4),
        (3, 0), (3, 1), (3, 2), (3, 3),
    ]

def transform_and_place(pattern, r_offset, c_offset, rotation=0, mirror=True):
    transformed = []
    for r, c in pattern:
        rr, cc = r, c
        if mirror:
            cc = -cc
        if rotation == 90:
            rr, cc = cc, -rr
        elif rotation == 180:
            rr, cc = -rr, -cc
        elif rotation == 270:
            rr, cc = -cc, rr
        transformed.append((rr + r_offset, cc + c_offset))
    return transformed

def write_bio_file(filename, rows, cols, population_limit, live_cells):
    uniq = set()
    for r, c, team in live_cells:
        if 0 <= r < rows and 0 <= c < cols:
            uniq.add((r, c, team))
    with open(filename, "w", encoding="utf-8") as f:
        f.write(f"{rows} {cols} {population_limit}\n")
        for r, c, team in sorted(uniq):
            f.write(f"{r} {c} {team}\n")

def ellipse_points(cy, cx, ry, rx, step_deg=14, start_deg=0, end_deg=360):
    pts = []
    if end_deg < start_deg:
        end_deg += 360
    a = start_deg
    while a <= end_deg:
        t = math.radians(a % 360)
        y = int(round(cy + ry * math.sin(t)))
        x = int(round(cx + rx * math.cos(t)))
        pts.append((y, x))
        a += step_deg
    return pts

def line_points(y0, x0, y1, x1, step=10):
    pts = []
    dy, dx = (y1 - y0), (x1 - x0)
    dist = max(1, int(math.hypot(dy, dx)))
    n = max(2, dist // step)
    for i in range(n + 1):
        t = i / n
        y = int(round(y0 + dy * t))
        x = int(round(x0 + dx * t))
        pts.append((y, x))
    return pts

def inside_ellipse(y, x, cy, cx, ry, rx, margin=0.0):
    """
    margin > 0 shrinks the usable interior (keeps away from rim).
    margin is expressed as a fraction of radii (e.g. 0.10).
    """
    ry2 = max(1e-9, (ry * (1.0 - margin)) ** 2)
    rx2 = max(1e-9, (rx * (1.0 - margin)) ** 2)
    dy = y - cy
    dx = x - cx
    return (dy * dy) / ry2 + (dx * dx) / rx2 <= 1.0

class StampSpacer:
    """
    Minimum spacing between LWSS anchor points.
    Anisotropic: Y-spacing is stricter to avoid collisions from vertical "flap".
    In X-direction you requested min distance 4 cells; we keep it >= 12 between anchors
    to be safe with the full LWSS footprint when used as "pixel".
    """
    def __init__(self, min_dx=12, min_dy=28, cell=10):
        self.min_dx = min_dx
        self.min_dy = min_dy
        self.cell = cell
        self.buckets = {}

    def _bucket(self, y, x):
        return (y // self.cell, x // self.cell)

    def can_place(self, y, x):
        by, bx = self._bucket(y, x)
        for yy in range(by - 1, by + 2):
            for xx in range(bx - 1, bx + 2):
                for (py, px) in self.buckets.get((yy, xx), []):
                    if abs(y - py) < self.min_dy and abs(x - px) < self.min_dx:
                        return False
        return True

    def add(self, y, x):
        b = self._bucket(y, x)
        self.buckets.setdefault(b, []).append((y, x))

def build_enterprise_lwss(rows=400, cols=400):
    """
    Produces a stylized Enterprise using LWSS stamps on a 400x400 grid.
    Color rule:
      - team 1: saucer + secondary hull ("cigar")
      - team 2: warp nacelles + pylons/struts + accents
    """
    # ---------------------------------------------------------
    # CENTRAL CONFIGURATION / ZENTRALE KONFIGURATION
    # ---------------------------------------------------------
    # Minimum spacing between LWSS stamps (in pixels)
    # X can be small (4), Y must be large (~28) to avoid vertical overlap/flapping.
    MIN_DX = 7
    MIN_DY = 28

    # Generation density (Oversampling settings)
    # We generate points very densely, then let StampSpacer filter them based on MIN_DX/DY.
    # This ensures that if you change MIN_DX, the lines automatically fill up.
    GEN_STEP_DEG = 3  # For ellipses (degrees)
    GEN_STEP_PX = 4   # For lines (pixels)
    # ---------------------------------------------------------

    cells = []

    # Composition anchor (roughly matches uploaded reference framing)
    base_cy = 165
    base_cx = 235

    # LWSS orientation: move left -> right
    ROT = 0
    MIR = True

    # Spacing: Use central config
    spacer = StampSpacer(min_dx=MIN_DX, min_dy=MIN_DY, cell=10)

    def stamp_lwss(y, x, team=1):
        if not (0 <= y < rows and 0 <= x < cols):
            return False
        if not spacer.can_place(y, x):
            return False
        pat = get_lwss_pattern()
        placed = transform_and_place(pat, y, x, rotation=ROT, mirror=MIR)
        for r, c in placed:
            if 0 <= r < rows and 0 <= c < cols:
                cells.append((r, c, team))
        spacer.add(y, x)
        return True

    # -------------------------
    # Saucer (Untertasse) - team 1
    # -------------------------
    saucer_ry = 50
    saucer_rx = 125

    # Outer rim
    # Increased density (step_deg 3) for clearer contour
    for (y, x) in ellipse_points(base_cy, base_cx, saucer_ry, saucer_rx, step_deg=GEN_STEP_DEG):
        stamp_lwss(y, x, team=1)

    # Inner ring (accent) - team 2 (keeps rule? user asked saucer same color as cigar,
    # but allowed two colors for contours. We'll keep saucer body team 1, accents team 2.)
    for (y, x) in ellipse_points(base_cy + 3, base_cx + 5, int(saucer_ry * 0.55), int(saucer_rx * 0.55), step_deg=20):
        stamp_lwss(y, x, team=2)

    # Bridge dome (accent)
    for (y, x) in ellipse_points(base_cy + 14, base_cx + 35, 9, 16, step_deg=45):
        stamp_lwss(y, x, team=2)

    # Underside trench arc (accent)
    for (y, x) in ellipse_points(base_cy + 10, base_cx + 25, 16, 50, step_deg=18, start_deg=210, end_deg=330):
        stamp_lwss(y, x, team=2)

    # Saucer interior texture (dithered, Y step respects spacing)
    for y in range(base_cy - 38, base_cy + 30, 10):
        for x in range(base_cx - 115, base_cx + 115, 16):
            if not inside_ellipse(y, x, base_cy, base_cx, saucer_ry, saucer_rx, margin=0.16):
                continue
            # carve out inner ring-ish region
            if inside_ellipse(y, x, base_cy + 3, base_cx + 5, int(saucer_ry * 0.55), int(saucer_rx * 0.55), margin=-0.03):
                continue
            if ((x // 8) + (y // 10)) % 3 == 0:
                stamp_lwss(y, x, team=1)

    # Light "window/panel" cluster to the right (accent)
    for dy in (-22, -10, 2):
        for dx in (70, 92, 112):
            stamp_lwss(base_cy + dy, base_cx + dx, team=2)

    # -------------------------
    # Neck (Hals) - structure accent team 2, body edges team 1
    # -------------------------
    neck_top_y = base_cy + 28
    neck_bot_y = base_cy + 85
    neck_left_x = base_cx - 18
    neck_right_x = base_cx + 18

    for (y, x) in line_points(neck_top_y, neck_left_x, neck_bot_y, neck_left_x - 14, step=GEN_STEP_PX):
        stamp_lwss(y, x, team=2)
    for (y, x) in line_points(neck_top_y, neck_right_x, neck_bot_y, neck_right_x - 8, step=GEN_STEP_PX):
        stamp_lwss(y, x, team=2)

    for y in range(neck_top_y + 10, neck_bot_y - 8, 18):
        stamp_lwss(y, base_cx + 2, team=1)

    # -------------------------
    # Secondary hull (Zigarre) - team 1
    # -------------------------
    hull_cy = base_cy + 118
    hull_cx = base_cx - 22
    hull_ry = 32
    hull_rx = 88

    for (y, x) in ellipse_points(hull_cy, hull_cx, hull_ry, hull_rx, step_deg=GEN_STEP_DEG):
        stamp_lwss(y, x, team=1)

    # Deflector dish (accent)
    for (y, x) in ellipse_points(hull_cy, hull_cx + 52, 14, 20, step_deg=28):
        stamp_lwss(y, x, team=2)

    # Hull texture (dithered)
    for y in range(hull_cy - 20, hull_cy + 21, 10):
        for x in range(hull_cx - 75, hull_cx + 70, 16):
            if not inside_ellipse(y, x, hull_cy, hull_cx, hull_ry, hull_rx, margin=0.14):
                continue
            if ((x // 8) - (y // 10)) % 4 == 0:
                stamp_lwss(y, x, team=1)

    # Windows rows (accent)
    for dx in range(-50, 60, 24):
        stamp_lwss(hull_cy - 10, hull_cx + dx, team=2)
    for dx in range(-40, 50, 24):
        stamp_lwss(hull_cy + 8, hull_cx + dx, team=2)

    # -------------------------
    # Pylons (Streben) - team 2
    # -------------------------
    pylon1 = line_points(hull_cy - 14, hull_cx - 6, hull_cy - 58, hull_cx - 105, step=GEN_STEP_PX)
    pylon2 = line_points(hull_cy - 10, hull_cx + 18, hull_cy - 52, hull_cx - 90, step=GEN_STEP_PX)
    for y, x in pylon1 + pylon2:
        stamp_lwss(y, x, team=2)

    # Inner brace accents
    pylon_inner = line_points(hull_cy - 20, hull_cx - 10, hull_cy - 60, hull_cx - 92, step=18)
    for y, x in pylon_inner:
        stamp_lwss(y, x, team=2)

    # -------------------------
    # Nacelles (Gondeln / Warp-Antriebe) - team 2
    # -------------------------
    n_ry = 16
    n_rx = 78

    # Upper nacelle
    n1_cy = base_cy + 55
    n1_cx = base_cx - 205
    for (y, x) in ellipse_points(n1_cy, n1_cx, n_ry, n_rx, step_deg=GEN_STEP_DEG):
        stamp_lwss(y, x, team=2)

    # Red cap-ish (front) accent
    for (y, x) in ellipse_points(n1_cy, n1_cx - 70, 11, 11, step_deg=45):
        stamp_lwss(y, x, team=2)

    # Lower nacelle
    n2_cy = base_cy + 105
    n2_cx = base_cx - 165
    for (y, x) in ellipse_points(n2_cy, n2_cx, n_ry, n_rx, step_deg=GEN_STEP_DEG):
        stamp_lwss(y, x, team=2)

    for (y, x) in ellipse_points(n2_cy, n2_cx + 70, 11, 11, step_deg=45):
        stamp_lwss(y, x, team=2)

    # Nacelle panel stripes (very sparse)
    for dx in range(-54, 55, 26):
        stamp_lwss(n1_cy - 4, n1_cx + dx, team=2)
        stamp_lwss(n2_cy + 4, n2_cx + dx, team=2)

    # -------------------------
    # Optional: a few sparse "sparkle" points to hint at surface detail (safe spacing already enforced)
    # -------------------------
    for (y, x) in [
        (base_cy - 8, base_cx - 40),
        (base_cy + 6, base_cx - 75),
        (hull_cy + 2, hull_cx - 10),
        (hull_cy - 16, hull_cx - 30),
    ]:
        stamp_lwss(y, x, team=2)

    return cells

if __name__ == "__main__":
    ROWS = 400
    COLS = 400
    POPULATION_LIMIT = 160000

    live_cells = build_enterprise_lwss(ROWS, COLS)

    if len(live_cells) > POPULATION_LIMIT:
        live_cells = live_cells[:POPULATION_LIMIT]

    write_bio_file("enterprise_400x400.bio", ROWS, COLS, POPULATION_LIMIT, live_cells)