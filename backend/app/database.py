import os
from motor.motor_asyncio import AsyncIOMotorClient
from dotenv import load_dotenv

# KI-Agent unterstützt: Database connection setup for MongoDB Atlas

load_dotenv()

MONGODB_URI = os.getenv("MONGODB_URI")
DB_NAME = "biotope_db"

if not MONGODB_URI:
    # In a production environment, you might want to handle this more gracefully
    # but for this challenge, the URI is mandatory.
    raise RuntimeError("MONGODB_URI environment variable is not set")

client = AsyncIOMotorClient(MONGODB_URI)
db = client[DB_NAME]


def get_db():
    """
    Returns the database instance.
    """
    return db
