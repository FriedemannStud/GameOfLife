from .models import Submission

# KI-Agent unterstützt: Business logic validation for Biotope rules


def validate_biotope_rules(submission: Submission):
    """
    Enforces the 'Fair Play' rules:
    1. Biomass limit: Max 24 cells (38% of 8x8 grid).
    2. Bounding Box: All cells within [0..7, 0..7].
    """
    cells = submission.config.cells

    # Rule 1: Biomass
    cell_count = len(cells)
    if cell_count > 24:
        raise ValueError(f"Biomass limit exceeded: {cell_count}/24 cells")

    # Rule 2: Bounding Box
    for x, y in cells:
        if not (0 <= x < 8 and 0 <= y < 8):
            raise ValueError(f"Cell ({x}, {y}) is outside the 8x8 bounding box")

    return True
