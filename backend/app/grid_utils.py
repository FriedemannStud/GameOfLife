from typing import Iterable, Sequence

# KI-Agent unterstützt: Shared sparse-cells -> dense 8x8 grid conversion (ADR-0025)


def cells_to_grid(cells: Iterable[Sequence[int]]) -> list:
    """Convert a sparse [x, y] cell list into a dense row-major 8x8 grid.

    Index convention matches the C side (grid_to_bitboard in game_logic.c and
    the renderer's seed[tr*8+tc]): index = y * 8 + x, with x = column, y = row.
    Out-of-bounds or malformed entries are ignored defensively.
    """
    grid = [0] * 64
    if not cells:
        return grid
    for cell in cells:
        if not cell or len(cell) < 2:
            continue
        x, y = int(cell[0]), int(cell[1])
        if 0 <= x < 8 and 0 <= y < 8:
            grid[y * 8 + x] = 1
    return grid
