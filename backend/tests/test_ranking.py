import sys
import os

# KI-Agent unterstützt: Path management for standalone test execution
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.ranking import calculate_elo

# KI-Agent unterstützt: Unit tests for Elo ranking logic

def test_elo_win():
    # New player (K=40) wins against equal opponent
    new_rating = calculate_elo(1200, 1200, 1.0, 0)
    assert new_rating == 1220
    print("test_elo_win passed")

def test_elo_loss():
    # New player (K=40) loses against equal opponent
    new_rating = calculate_elo(1200, 1200, 0.0, 0)
    assert new_rating == 1180
    print("test_elo_loss passed")

def test_elo_draw():
    # New player (K=40) draws against equal opponent
    new_rating = calculate_elo(1200, 1200, 0.5, 0)
    assert new_rating == 1200
    print("test_elo_draw passed")

def test_elo_k_factor_switch():
    # Established player (K=20) wins against equal opponent
    new_rating = calculate_elo(1200, 1200, 1.0, 10)
    assert new_rating == 1210
    print("test_elo_k_factor_switch passed")

def test_elo_stronger_opponent():
    # Player wins against much stronger opponent (Expected score is low)
    # R_a=1200, R_b=1600 -> Expected_a ~ 0.09
    # New rating: 1200 + 40 * (1.0 - 0.09) = 1200 + 36.4 = 1236
    new_rating = calculate_elo(1200, 1600, 1.0, 0)
    assert new_rating == 1236
    print("test_elo_stronger_opponent passed")

if __name__ == "__main__":
    print("Running Elo unit tests...")
    test_elo_win()
    test_elo_loss()
    test_elo_draw()
    test_elo_k_factor_switch()
    test_elo_stronger_opponent()
    print("All Elo tests passed!")
