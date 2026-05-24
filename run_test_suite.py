import json
import random
import subprocess
import time
import os

def create_test_file(filename, competitors, max_gen=1000):
    with open(filename, "w") as f:
        json.dump({
            "max_generations": max_gen,
            "competitors": competitors
        }, f, indent=2)

def run_test(test_name, input_file, output_file):
    print(f"\n--- Running Test: {test_name} ---")
    start = time.time()
    try:
        result = subprocess.run(
            ["./biotope_hyper_worker", input_file, output_file],
            capture_output=True, text=True, timeout=120
        )
        duration = time.time() - start
        
        if result.returncode != 0:
            print(f"FAILED (Return Code {result.returncode})")
            print(result.stderr)
            return {"status": "FAILED", "duration": duration, "error": result.stderr}
            
        with open(output_file, "r") as f:
            out_data = json.load(f)
            
        print(f"SUCCESS: {out_data['total_matches_played']} matches in {duration:.2f}s (Real). CPU Time: {out_data['execution_time_cpu_s']:.2f}s")
        return {"status": "SUCCESS", "duration": duration, "cpu_time": out_data['execution_time_cpu_s']}
    except subprocess.TimeoutExpired:
        print("TIMEOUT (>120s)")
        return {"status": "TIMEOUT", "duration": 120}


def run_e2e_test(test_name):
    print(f"\n--- Running Test: {test_name} ---")
    start = time.time()
    try:
        result = subprocess.run(
            ["./biotope_headless", "tests/tc6_e2e_headless_red.json", "tests/tc6_e2e_headless_blue.json", "tests/out_tc6.json"],
            capture_output=True, text=True, timeout=10
        )
        duration = time.time() - start
        
        if result.returncode != 0:
            print(f"FAILED (Return Code {result.returncode})")
            print(result.stderr)
            return {"status": "FAILED", "duration": duration, "error": result.stderr}
            
        with open("tests/out_tc6.json", "r") as f:
            out_data = json.load(f)
        with open("tests/golden_snapshot.json", "r") as f:
            golden_data = json.load(f)
            
        # Ignore timestamp for comparison
        if "timestamp" in out_data: del out_data["timestamp"]
        if "timestamp" in golden_data: del golden_data["timestamp"]
        
        if out_data == golden_data:
            print(f"SUCCESS: Bit-for-bit identical to golden snapshot in {duration:.2f}s.")
            return {"status": "SUCCESS", "duration": duration, "cpu_time": 0}
        else:
            print("FAILED: Output does not match golden snapshot.")
            print(f"Expected: {golden_data}\nGot: {out_data}")
            return {"status": "FAILED", "duration": duration, "error": "Mismatch with golden snapshot"}
    except subprocess.TimeoutExpired:
        print("TIMEOUT (>10s)")
        return {"status": "TIMEOUT", "duration": 10}

# 1. Standard
comps_std = []
for i in range(100):
    comps_std.append({"player_id": f"p_{i}", "cells": [[random.randint(0,7), random.randint(0,7)] for _ in range(10)]})
create_test_file("tc1_std.json", comps_std)

# 2. Still Life (Early Termination Test)
comps_still = []
for i in range(500):
    # 2x2 Block is a still life
    comps_still.append({"player_id": f"p_{i}", "cells": [[0,0],[0,1],[1,0],[1,1]]})
create_test_file("tc2_still.json", comps_still)

# 3. Chaos (Long running)
comps_chaos = []
for i in range(200):
    # R-pentomino or random dense
    comps_chaos.append({"player_id": f"p_{i}", "cells": [[1,2],[2,1],[2,2],[2,3],[3,1]]})
create_test_file("tc3_chaos.json", comps_chaos, max_gen=1000)

# 4. Robustness (Invalid coords)
comps_invalid = [
    {"player_id": "out_of_bounds_pos", "cells": [[8,8], [100, 100], [0,0]]},
    {"player_id": "out_of_bounds_neg", "cells": [[-1,-1], [0,0]]},
    {"player_id": "normal", "cells": [[1,1], [1,2], [2,1], [2,2]]}
]
create_test_file("tc4_invalid.json", comps_invalid)

# 5. The Meltdown (1000 players, max_gen 2000, random dense)
comps_meltdown = []
for i in range(1000):
    comps_meltdown.append({"player_id": f"p_{i}", "cells": [[random.randint(0,7), random.randint(0,7)] for _ in range(15)]})
create_test_file("tc5_meltdown.json", comps_meltdown, max_gen=2000)

results = {}
results["TC-01 Standard (100)"] = run_test("TC-01 Standard Load", "tc1_std.json", "out_tc1.json")
results["TC-02 Still-Life (500)"] = run_test("TC-02 Early Termination", "tc2_still.json", "out_tc2.json")
results["TC-03 Chaos (200)"] = run_test("TC-03 Long Running", "tc3_chaos.json", "out_tc3.json")
results["TC-04 Robustness"] = run_test("TC-04 Invalid Inputs", "tc4_invalid.json", "out_tc4.json")
results["TC-05 Meltdown (1000, 2000gen)"] = run_test("TC-05 Meltdown", "tc5_meltdown.json", "out_tc5.json")
results["TC-06 End-to-End System DEV_TEST"] = run_e2e_test("TC-06 End-to-End System DEV_TEST")

print("\n--- SUMMARY ---")
for k, v in results.items():
    if v['status'] == 'SUCCESS':
        print(f"{k}: {v['duration']:.2f}s Real / {v.get('cpu_time', 0):.2f}s CPU")
    else:
        print(f"{k}: {v['status']}")

