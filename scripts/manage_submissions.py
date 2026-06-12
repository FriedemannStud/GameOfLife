#!/usr/bin/env python3
# KI-Agent unterstützt
"""
Interactive CLI to inspect and selectively delete submission records from MongoDB.
Deletes the N most recently submitted configurations (newest first).
"""

import asyncio
import os
import sys
from dotenv import load_dotenv
from motor.motor_asyncio import AsyncIOMotorClient

load_dotenv()

MONGODB_URI = os.getenv("MONGODB_URI")
DB_NAME = os.getenv("MONGODB_DB", "biotope_db")


async def main():
    if not MONGODB_URI:
        print("Error: MONGODB_URI not set in .env")
        sys.exit(1)

    try:
        client = AsyncIOMotorClient(MONGODB_URI, serverSelectionTimeoutMS=5000)
        db = client[DB_NAME]
        await db.command("ping")
    except Exception as e:
        print(f"Connection failed: {e}")
        sys.exit(1)

    col = db["submissions"]

    total = await col.count_documents({})
    print(f"\nDatenbank: {DB_NAME} | Collection: submissions")
    print(f"Vorhandene Start-Konfigurationen: {total}\n")

    if total == 0:
        print("Keine Datensätze vorhanden. Nichts zu löschen.")
        client.close()
        return

    print("Wie viele Datensätze sollen gelöscht werden (jüngste zuerst)?")
    print(f"  Eingabe 0 = nichts löschen | {total} = alle löschen")
    try:
        raw = input(f"  Anzahl [0-{total}]: ").strip()
        n = int(raw)
    except (ValueError, EOFError):
        print("Ungültige Eingabe. Abbruch.")
        client.close()
        return

    if n < 0 or n > total:
        print(f"Ungültiger Wert. Muss zwischen 0 und {total} liegen. Abbruch.")
        client.close()
        return

    if n == 0:
        print("Nichts gelöscht.")
        client.close()
        return

    # Collect _id of the N newest documents (sorted descending by _id, which embeds timestamp)
    cursor = col.find({}, {"_id": 1}).sort("_id", -1).limit(n)
    ids_to_delete = [doc["_id"] async for doc in cursor]

    result = await col.delete_many({"_id": {"$in": ids_to_delete}})

    remaining = await col.count_documents({})
    print(f"\n{result.deleted_count} Datensatz/Datensätze gelöscht.")
    print(f"Verbleibende Start-Konfigurationen: {remaining}\n")

    client.close()


if __name__ == "__main__":
    asyncio.run(main())
