#!/usr/bin/env python3
"""
backup_mongo.py — Dump the biotope MongoDB to backup/mongodump_<timestamp>/

Run this on the SERVER (outside Docker) while the stack is up:

    python scripts/backup_mongo.py

mongodump runs inside the running mongo container (no host installation needed).
Credentials are read from the project's .env file (MONGO_USER, MONGO_PASSWORD).
The dump lands in backup/mongodump_YYYYMMDD_HHMMSS/ relative to the project root.

After the dump, commit and push to preserve the data in git:

    git add backup/
    git commit -m "backup: MongoDB dump YYYY-MM-DD"
    git push

To restore on a fresh machine with a running stack (mongorestore must be available):

    mongorestore --host 127.0.0.1 --port 27018 \\
        --username $MONGO_USER --password $MONGO_PASSWORD \\
        --authenticationDatabase admin \\
        backup/mongodump_<timestamp>/biotope_db
"""

import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BACKUP_BASE = ROOT / "backup"
ENV_FILE = ROOT / ".env"


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


def find_mongo_container() -> str:
    """Return the name of the running mongo container from the compose stack."""
    out = subprocess.check_output(
        [
            "docker", "ps",
            "--filter", "label=com.docker.compose.service=mongo",
            "--format", "{{.Names}}",
        ],
        text=True,
    ).strip()
    if not out:
        raise RuntimeError(
            "No running 'mongo' container found. Is 'docker compose up' running?"
        )
    return out.splitlines()[0]


def main() -> None:
    env = load_env(ENV_FILE)

    user = env.get("MONGO_USER") or os.getenv("MONGO_USER")
    password = env.get("MONGO_PASSWORD") or os.getenv("MONGO_PASSWORD")
    db_name = env.get("MONGODB_DB") or os.getenv("MONGODB_DB") or "biotope_db"

    if not user or not password:
        print("ERROR: MONGO_USER and MONGO_PASSWORD must be set in .env or environment.")
        sys.exit(1)

    try:
        container = find_mongo_container()
    except RuntimeError as e:
        print(f"ERROR: {e}")
        sys.exit(1)

    print(f"Using container: {container}")

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    inner_dump = f"/tmp/mongodump_{timestamp}"
    out_dir = BACKUP_BASE / f"mongodump_{timestamp}"

    print(f"Dumping '{db_name}' → {out_dir.relative_to(ROOT)}\n")

    # Run mongodump inside the container (mongodump connects to localhost:27017 there)
    dump_cmd = [
        "docker", "exec", container,
        "mongodump",
        "--username", user,
        "--password", password,
        "--authenticationDatabase", "admin",
        "--db", db_name,
        "--out", inner_dump,
    ]
    result = subprocess.run(dump_cmd)
    if result.returncode != 0:
        print("\nERROR: mongodump failed inside the container.")
        sys.exit(result.returncode)

    # Copy dump from container to host backup directory
    BACKUP_BASE.mkdir(parents=True, exist_ok=True)
    subprocess.run(["docker", "cp", f"{container}:{inner_dump}", str(BACKUP_BASE)], check=True)

    # Clean up the temporary dump inside the container
    subprocess.run(["docker", "exec", container, "rm", "-rf", inner_dump])

    total_bytes = sum(f.stat().st_size for f in out_dir.rglob("*") if f.is_file())
    print(f"\nDump complete. Size: {total_bytes / 1024:.1f} KB")
    print("\nNext steps on the server:")
    print(f"  git add backup/")
    print(f"  git commit -m 'backup: MongoDB dump {timestamp[:8]}'")
    print(f"  git push")


if __name__ == "__main__":
    main()
