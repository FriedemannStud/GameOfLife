from fastapi import FastAPI, HTTPException
from datetime import datetime
from .models import Submission
from .validators import validate_biotope_rules
from .storage import save_submission

# KI-Agent unterstützt: Final implementation of Biotope Backend
app = FastAPI(title="Biotope API")


@app.get("/")
async def root():
    return {"message": "Hello Biotope"}


@app.post("/api/v1/submit_config", status_code=201)
async def submit_config(submission: Submission):
    try:
        # 1. Validate Business Rules (Fair Play)
        validate_biotope_rules(submission)

        # 2. Persist for C-Worker
        filename = save_submission(submission)

        return {
            "status": "success",
            "submission_id": filename,
            "timestamp": datetime.now().isoformat(),
        }
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception:
        raise HTTPException(status_code=500, detail="Internal server error")
