#!/usr/bin/env python3
# KI-Agent unterstützt: Performance-instrumented round-robin runner.
#
# Runs the C hyper-worker over all active submissions, measures how the
# tournament scales, writes one metrics document per run into the
# `performance_metrics` collection (live dashboard + later analysis), and writes
# the rankings back into `submissions` (same fields as worker.execute_epoch) so
# the configs are actually ranked against each other.
#
# Why this exists: during the fair, humans keep submitting start patterns. A
# pattern that never stabilises forces every one of its matches to run the full
# `max_generations` — the dominant performance risk. This tool surfaces exactly
# that (per-run throughput + the most expensive, longest-living configs).
#
# Usage (host, hyper_worker must be built via `make`):
#   python3 backend/scripts/perf_tournament.py
#   python3 backend/scripts/perf_tournament.py --max-gen 1000 --no-writeback
#
# Reads MONGODB_URI / MONGODB_DB from repo-root .env (rewrites docker host).

import json
import os
import resource
import statistics as st
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone

from dotenv import load_dotenv
from pymongo import MongoClient, UpdateOne

PERF_COLLECTION = "performance_metrics"

HYPER_PATHS = [
    "./build/biotope_hyper_worker",
    "./worker_bin/biotope_hyper_worker",
    "/app/build/biotope_hyper_worker",
]


def connect():
    here = os.path.dirname(os.path.abspath(__file__))
    load_dotenv(os.path.join(here, "..", "..", ".env"))
    uri = os.getenv("TOP_PATTERNS_URI") or os.getenv("MONGODB_URI")
    if not uri:
        sys.exit("MONGODB_URI not set in repo-root .env")
    uri = uri.replace("@mongo:27017", "@127.0.0.1:27018")
    db_name = os.getenv("MONGODB_DB", "biotope_db")
    return MongoClient(uri, serverSelectionTimeoutMS=4000)[db_name], db_name


def pct(values, p):
    if not values:
        return 0.0
    s = sorted(values)
    k = max(0, min(len(s) - 1, int(round((p / 100.0) * (len(s) - 1)))))
    return s[k]


def main():
    args = sys.argv[1:]
    max_gen = 1000
    if "--max-gen" in args:
        max_gen = int(args[args.index("--max-gen") + 1])
    writeback = "--no-writeback" not in args

    db, db_name = connect()
    binary = next((p for p in HYPER_PATHS if os.path.exists(p)), None)
    if not binary:
        sys.exit(f"hyper_worker not found in {HYPER_PATHS}. Run `make` first.")

    subs = list(
        db.submissions.find(
            {"status": "active"},
            {"config.cells": 1, "metadata.nickname": 1, "config.cell_count": 1},
        )
    )
    if not subs:
        sys.exit(f"No active submissions in '{db_name}'.")
    id_to_meta = {
        str(s["_id"]): (
            s.get("metadata", {}).get("nickname", "?"),
            s.get("config", {}).get("cell_count"),
        )
        for s in subs
    }

    batch = {
        "max_generations": max_gen,
        "competitors": [
            {"player_id": str(s["_id"]), "cells": s["config"]["cells"]} for s in subs
        ],
    }

    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as f:
        json.dump(batch, f)
        in_path = f.name
    out_path = in_path + ".out.json"

    print(
        f"[{db_name}] running hyper_worker over {len(subs)} competitors "
        f"(max_gen={max_gen}) ..."
    )
    cpu_before = resource.getrusage(resource.RUSAGE_CHILDREN)
    wall0 = time.monotonic()
    proc = subprocess.run([binary, in_path, out_path], capture_output=True, text=True)
    wall_s = time.monotonic() - wall0
    cpu_after = resource.getrusage(resource.RUSAGE_CHILDREN)
    child_cpu_s = (cpu_after.ru_utime + cpu_after.ru_stime) - (
        cpu_before.ru_utime + cpu_before.ru_stime
    )

    if proc.returncode != 0:
        sys.exit(f"hyper_worker failed (rc={proc.returncode}): {proc.stderr}")
    with open(out_path) as f:
        results = json.load(f)

    rankings = results.get("rankings", [])
    total_matches = results.get("total_matches_played", 0)
    cpu_s = results.get("execution_time_cpu_s", child_cpu_s) or child_cpu_s

    stable = [r.get("avg_stable_generation", 0.0) for r in rankings]
    n_never = sum(1 for v in stable if v >= max_gen - 1)  # hit the gen cap
    expensive = sorted(
        rankings, key=lambda r: r.get("avg_stable_generation", 0.0), reverse=True
    )[:10]
    top_expensive = [
        {
            "nickname": id_to_meta.get(r["player_id"], ("?", None))[0],
            "cell_count": id_to_meta.get(r["player_id"], ("?", None))[1],
            "avg_stable_generation": round(r.get("avg_stable_generation", 0.0), 1),
            "win_rate": round(r.get("win_rate", 0.0) * 100, 1),
        }
        for r in expensive
    ]

    doc = {
        "timestamp": datetime.now(timezone.utc),
        "dataset": db_name,
        "n_competitors": len(subs),
        "max_generations": max_gen,
        "total_matches_played": total_matches,
        "wall_clock_s": round(wall_s, 2),
        "cpu_time_s": round(cpu_s, 2),
        "parallel_speedup": round(cpu_s / wall_s, 1) if wall_s else None,
        "matches_per_s": round(total_matches / wall_s, 1) if wall_s else None,
        "us_per_match": (
            round(wall_s * 1e6 / total_matches, 2) if total_matches else None
        ),
        "stable_gen_mean": round(st.mean(stable), 1) if stable else 0,
        "stable_gen_p50": round(pct(stable, 50), 1),
        "stable_gen_p95": round(pct(stable, 95), 1),
        "stable_gen_max": round(max(stable), 1) if stable else 0,
        "n_never_stabilized": n_never,
        "pct_never_stabilized": (
            round(100 * n_never / len(rankings), 1) if rankings else 0
        ),
        "top_expensive_configs": top_expensive,
    }
    db[PERF_COLLECTION].insert_one(doc)

    # Write rankings back so the configs are actually ranked (mirrors worker)
    if writeback and rankings:
        ops = []
        from bson import ObjectId

        for pos, r in enumerate(rankings, start=1):
            ops.append(
                UpdateOne(
                    {"_id": ObjectId(r["player_id"])},
                    {
                        "$set": {
                            "rank": pos,
                            "win_rate": r.get("win_rate", 0.0),
                            "wins": r.get("wins", 0),
                            "draws": r.get("draws", 0),
                            "losses": r.get("losses", 0),
                            "total_score": r.get("total_score", 0),
                            "matches_played": r.get("matches_played", 0),
                            "avg_stable_generation": r.get(
                                "avg_stable_generation", 0.0
                            ),
                            "last_epoch_at": datetime.now(timezone.utc),
                        }
                    },
                )
            )
        db.submissions.bulk_write(ops, ordered=False)

    os.unlink(in_path)
    os.unlink(out_path)

    print(f"\n=== Performance run [{db_name}] ===")
    print(f"  competitors      : {doc['n_competitors']}")
    print(f"  matches played   : {doc['total_matches_played']:,}")
    print(f"  wall clock       : {doc['wall_clock_s']} s")
    print(
        f"  cpu time         : {doc['cpu_time_s']} s  "
        f"(speedup {doc['parallel_speedup']}x)"
    )
    print(
        f"  throughput       : {doc['matches_per_s']:,} matches/s  "
        f"({doc['us_per_match']} µs/match)"
    )
    print(
        f"  stable-gen p50/p95/max: {doc['stable_gen_p50']}/"
        f"{doc['stable_gen_p95']}/{doc['stable_gen_max']}"
    )
    print(
        f"  never stabilized : {doc['n_never_stabilized']} "
        f"({doc['pct_never_stabilized']}%)  <- perf risk"
    )
    print(
        f"  most expensive   : {top_expensive[0]['nickname']} "
        f"(avg-stable-gen {top_expensive[0]['avg_stable_generation']})"
    )
    print(
        f"  -> written to '{db_name}.{PERF_COLLECTION}'"
        f"{' + rankings written back' if writeback else ''}"
    )


if __name__ == "__main__":
    main()
