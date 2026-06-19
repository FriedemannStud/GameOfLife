import os

from dotenv import load_dotenv
from motor.motor_asyncio import AsyncIOMotorClient

# KI-Agent unterstützt: Database connection setup for MongoDB

load_dotenv()

MONGODB_URI = os.getenv("MONGODB_URI")
DB_NAME = os.getenv("MONGODB_DB", "biotope_db")

if not MONGODB_URI:
    raise RuntimeError("MONGODB_URI environment variable is not set in .env")

client = AsyncIOMotorClient(MONGODB_URI)
db = client[DB_NAME]

# Collections
submissions_col = db["submissions"]
players_col = db["players"]

# KI-Agent unterstützt: Incremental tournament collections (ADR-0032).
#
# worker_state — singleton control docs for the tournament worker.
#   _id: "epoch_guard"  — { fingerprint: str, updated_at: datetime }
#   Stores the roster fingerprint of the last *successful* epoch so an
#   unchanged active set is skipped (Stage 1).
#
# match_results — content-addressed cache of deterministic match outcomes
#   (Stage 2). Keyed by the *unordered* pair of seed hashes; valid only while
#   matches stay deterministic and the ADR-0026 orientation symmetry holds
#   (see the determinism tripwire in worker.py). Schema:
#     pair_key:              str   "<min_hash>:<max_hash>"  (unique index)
#     hash_a, hash_b:        str   the two seed_hash values (hash_a <= hash_b)
#     winner:                str   "a" | "b" | "draw"
#     pop_a, pop_b:          int   final population of each seed's team
#     activity_sum:          int   total births+deaths over the match
#     stable_at_generation:  int   generation the match converged (0 if none)
#     computed_at:           datetime
worker_state_col = db["worker_state"]
match_results_col = db["match_results"]


async def check_connection():
    """
    Pings the MongoDB server to verify credentials and connectivity.
    """
    try:
        await db.command("ping")
        return True
    except Exception as e:
        print(f"MongoDB Connection Error: {e}")
        return False


def get_db():
    """
    Returns the database instance.
    """
    return db
