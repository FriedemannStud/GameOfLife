import subprocess
import os
import json
import tempfile
import sys

# KI-Agent unterstützt: System integration tests for C-Worker and Environment
# This test ensures that the C binaries are executable and have all dependencies.

BINARY_PATH = "./build/biotope_hyper_worker"

def test_binary_exists():
    assert os.path.exists(BINARY_PATH), f"Binary {BINARY_PATH} not found. Run 'make' first."

def test_binary_dependencies():
    """
    Checks if all dynamic libraries are resolved (no 'not found' in ldd).
    This catches issues like missing libcurl or libgomp in the execution environment.
    """
    if sys.platform == "win32":
        print("Skipping ldd test on Windows")
        return
        
    try:
        result = subprocess.run(["ldd", BINARY_PATH], capture_output=True, text=True, check=True)
        if "not found" in result.stdout:
            print(f"FAILED: Missing dependencies for {BINARY_PATH}:\n{result.stdout}")
            sys.exit(1)
        print("Binary dependencies: OK")
    except FileNotFoundError:
        print("ldd command not found. Skipping dependency check.")

def test_hyper_worker_execution():
    """
    Runs the hyper worker with a minimal valid JSON input and verifies valid output.
    This ensures permissions, paths, and basic logic are working.
    """
    input_data = {
        "max_generations": 100,
        "competitors": [
            {"player_id": "test_1", "cells": [[0,0], [0,1], [1,0]]},
            {"player_id": "test_2", "cells": [[7,7], [7,6], [6,7]]}
        ]
    }
    
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f_in:
        json.dump(input_data, f_in)
        input_path = f_in.name
    
    output_path = input_path + ".out.json"
    
    try:
        # Run the binary
        result = subprocess.run([BINARY_PATH, input_path, output_path], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"FAILED: Worker execution failed (Exit code {result.returncode})")
            print(f"Stderr: {result.stderr}")
            sys.exit(1)
            
        # Verify output
        if not os.path.exists(output_path):
            print("FAILED: Output file was not created.")
            sys.exit(1)
            
        with open(output_path, "r") as f_out:
            data = json.load(f_out)
            if "rankings" not in data or len(data["rankings"]) != 2:
                print(f"FAILED: Invalid output format: {data}")
                sys.exit(1)
            if "total_matches_played" not in data or data["total_matches_played"] == 0:
                print("FAILED: No matches were played.")
                sys.exit(1)
        
        print("Hyper-Worker execution: OK")
        
    finally:
        # Cleanup
        if os.path.exists(input_path): os.remove(input_path)
        if os.path.exists(output_path): os.remove(output_path)

if __name__ == "__main__":
    print("=== Starting System Integration Tests ===")
    test_binary_exists()
    test_binary_dependencies()
    test_hyper_worker_execution()
    print("=== All System Integration Tests PASSED ===")
