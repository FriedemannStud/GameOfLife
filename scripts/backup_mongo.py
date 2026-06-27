#!/usr/bin/env python3
"""
backup_mongo.py — Dump the biotope MongoDB to backup/mongodump_<timestamp>/

Run this on the SERVER (outside Docker) while the stack is up:

    python scripts/backup_mongo.py

Credentials are read from the project's .env file (MONGO_USER, MONGO_PASSWORD).
The dump lands in backup/mongodump_YYYYMMDD_HHMMSS/ relative to the project root.

After the dump, commit and push to preserve the data in git:

    git add backup/
    git commit -m "backup: MongoDB dump YYYY-MM-DD"
    git push

To restore on a fresh machine with a running stack:

    mongorestore --host 127.0.0.1 --port 27018 \\
        --username $MONGO_USER --password $MONGO_PASSWORD \\
        --authenticationDatabase admin \\
        backup/mongodump_<timestamp>/
"""

import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BACKUP_BASE = ROOT / "backup"
ENV_FILE = ROOT / ".env"

MONGO_HOST = "127.0.0.1"
MONGO_PORT = "27018"  # host-side port from docker-compose


def load_env(path: Path) -> dict:
    """Parse key=value pairs from .env, ignoring comments and blank lines."""
    env = {}
    if not path.exists():
        return env
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" in line:
            key, _, value = line.partition("=")
            env[key.strip()] = value.strip()
    return env


def main() -> None:
    env = load_env(ENV_FILE)

    user = env.get("MONGO_USER") or os.getenv("MONGO_USER")
    password = env.get("MONGO_PASSWORD") or os.getenv("MONGO_PASSWORD")
    db_name = env.get("MONGODB_DB") or os.getenv("MONGODB_DB") or "biotope_db"

    if not user or not password:
        print("ERROR: MONGO_USER and MONGO_PASSWORD must be set in .env or environment.")
        sys.exit(1)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    out_dir = BACKUP_BASE / f"mongodump_{timestamp}"
    out_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        "mongodump",
        "--host", MONGO_HOST,
        "--port", MONGO_PORT,
        "--username", user,
        "--password", password,
        "--authenticationDatabase", "admin",
        "--db", db_name,
        "--out", str(out_dir),
    ]

    print(f"Connecting to {MONGO_HOST}:{MONGO_PORT}, database '{db_name}' ...")
    print(f"Output → {out_dir.relative_to(ROOT)}\n")

    result = subprocess.run(cmd)

    if result.returncode != 0:
        print("\nERROR: mongodump exited with a non-zero status.")
        sys.exit(result.returncode)

    total_bytes = sum(f.stat().st_size for f in out_dir.rglob("*") if f.is_file())
    print(f"\nDump complete. Size: {total_bytes / 1024:.1f} KB")
    print("\nNext steps on the server:")
    print(f"  git add backup/")
    print(f"  git commit -m 'backup: MongoDB dump {timestamp[:8]}'")
    print(f"  git push")


if __name__ == "__main__":
    main()
