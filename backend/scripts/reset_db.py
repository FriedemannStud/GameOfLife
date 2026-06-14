import asyncio
import os
import sys

# Add backend root to path so `app` is importable when run directly.
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import DB_NAME, get_db

# KI-Agent unterstützt: Clean-slate reset before the fair (ADR-0027, FR-11).
# Clears players + submissions and (re)creates the unique ownership index.
# DESTRUCTIVE — run deliberately and only against the event database.


async def main():
    db = get_db()
    print(f"Resetting database: {DB_NAME}")

    players_result = await db.players.delete_many({})
    submissions_result = await db.submissions.delete_many({})
    await db.players.create_index("nickname_normalized", unique=True)

    print(f"  players deleted:     {players_result.deleted_count}")
    print(f"  submissions deleted: {submissions_result.deleted_count}")
    print("  unique index on players.nickname_normalized ensured")
    print("Done — clean slate ready.")


if __name__ == "__main__":
    asyncio.run(main())
