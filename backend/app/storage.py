import os
from datetime import datetime
from .models import Submission

# KI-Agent unterstützt: Persistence layer for Biotope submissions

RESULTS_DIR = "/app/results"


def save_submission(submission: Submission) -> str:
    """
    Saves the submission as a JSON file for the C-worker.
    Returns the generated filename.
    """
    if not os.path.exists(RESULTS_DIR):
        os.makedirs(RESULTS_DIR)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    filename = f"submission_{timestamp}.json"
    file_path = os.path.join(RESULTS_DIR, filename)

    # We use model_dump_json() for clean JSON output from Pydantic
    with open(file_path, "w") as f:
        f.write(submission.model_dump_json(indent=2))

    return filename
