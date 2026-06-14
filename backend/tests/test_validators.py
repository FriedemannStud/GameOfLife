import os
import sys

# Add app to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import Config, Metadata, Submission
from app.validators import validate_biotope_rules

# KI-Agent unterstützt: Simple test script for validators


def test_validators():
    print("Running validator tests...")

    # 1. Valid Pattern
    valid_sub = Submission(
        metadata=Metadata(player_id="u1", nickname="n1"),
        config=Config(cells=[(0, 0), (7, 7)]),
    )
    assert validate_biotope_rules(valid_sub)
    print("OK: Valid pattern passed.")

    # 2. Too many cells (25)
    too_many_cells = [(i % 8, i // 8) for i in range(25)]
    invalid_sub_biomass = Submission(
        metadata=Metadata(player_id="u1", nickname="n1"),
        config=Config(cells=too_many_cells),
    )
    try:
        validate_biotope_rules(invalid_sub_biomass)
        assert False, "Should have raised ValueError for biomass"
    except ValueError as e:
        print(f"OK: Caught expected biomass error: {e}")

    # 3. Out of bounds (8, 0)
    invalid_sub_bounds = Submission(
        metadata=Metadata(player_id="u1", nickname="n1"), config=Config(cells=[(8, 0)])
    )
    try:
        validate_biotope_rules(invalid_sub_bounds)
        assert False, "Should have raised ValueError for bounds"
    except ValueError as e:
        print(f"OK: Caught expected bounds error: {e}")


if __name__ == "__main__":
    test_validators()
