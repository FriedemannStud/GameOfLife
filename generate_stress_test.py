import json
import random

def generate_stress_test(num_competitors, max_gen=1000):
    test_data = {
        "max_generations": max_gen,
        "competitors": []
    }
    
    for i in range(num_competitors):
        num_cells = random.randint(5, 20)
        cells = []
        for _ in range(num_cells):
            x = random.randint(0, 7)
            y = random.randint(0, 7)
            if [x, y] not in cells:
                cells.append([x, y])
        
        test_data["competitors"].append({
            "player_id": f"bot_{i:03d}",
            "cells": cells
        })
    
    with open(f"stress_test_{num_competitors}.json", "w") as f:
        json.dump(test_data, f, indent=2)
    print(f"Generated stress_test_{num_competitors}.json with {num_competitors} competitors.")

if __name__ == "__main__":
    generate_stress_test(100) # Quick stress test
    generate_stress_test(300) # Medium stress test
