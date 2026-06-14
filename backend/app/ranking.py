# KI-Agent unterstützt: Elo calculation logic for Biotope Matchmaking


def calculate_elo(
    rating_a: int, rating_b: int, score_a: float, matches_played_a: int
) -> int:
    """
    Calculates the new Elo rating for player A.

    Args:
        rating_a: Current Elo rating of player A.
        rating_b: Current Elo rating of player B.
        score_a: Outcome for player A (1.0 for win, 0.5 for draw, 0.0 for loss).
        matches_played_a: Number of matches player A has played (used for the
            dynamic K-factor).

    Returns:
        The new Elo rating for player A as an integer.
    """
    # 1. Calculate expected score for A
    # E_a = 1 / (1 + 10^((R_b - R_a) / 400))
    expected_a = 1 / (1 + 10 ** ((rating_b - rating_a) / 400))

    # 2. Determine K-factor (Dynamic)
    # Higher K-factor for new players to allow faster ranking convergence
    k_factor = 40 if matches_played_a < 10 else 20

    # 3. Calculate new rating
    # R'_a = R_a + K * (S_a - E_a)
    new_rating = rating_a + k_factor * (score_a - expected_a)

    return int(round(new_rating))
