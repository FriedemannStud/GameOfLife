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
