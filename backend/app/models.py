from pydantic import BaseModel, Field
from typing import List, Tuple, Optional
from datetime import datetime

# KI-Agent unterstützt: Extended Pydantic models for MongoDB persistence and Matchmaking


class Metadata(BaseModel):
    player_id: str = Field(
        ..., description="Unique ID of the player", examples=["user_123"]
    )
    nickname: str = Field(
        ..., description="Player's display name", examples=["VibeMaster"]
    )
    league: str = Field("local", description="Competition league", examples=["local"])


class Config(BaseModel):
    bounding_box_x: int = Field(
        8, description="Width of the pattern area", ge=8, le=8, examples=[8]
    )
    bounding_box_y: int = Field(
        8, description="Height of the pattern area", ge=8, le=8, examples=[8]
    )
    cells: List[Tuple[int, int]] = Field(
        ...,
        description="List of relative [x, y] coordinates. Max 24 cells allowed.",
        examples=[[(0, 0), (1, 1), (2, 2)]],
    )


class Auth(BaseModel):
    # KI-Agent unterstützt: Top-level secret carrier (ADR-0027); never persisted.
    recovery_code: Optional[str] = Field(
        None, description="Recovery code, only sent during a reclaim"
    )


class SubmissionBase(BaseModel):
    metadata: Metadata
    config: Config


class Submission(SubmissionBase):
    # KI-Agent unterstützt: API payload — `auth` is structurally isolated from the
    # persisted document. Defaults to empty so the existing metadata/config payload
    # stays valid.
    auth: Auth = Field(default_factory=Auth)


class DBSubmission(SubmissionBase):
    # KI-Agent unterstützt: Persisted shape inherits only metadata + config from
    # SubmissionBase, so no model_dump() path can leak the recovery code (FR-7).
    status: str = Field(
        "active", description="Matchmaking status: active, in_match, retired"
    )
    elo_rating: int = Field(1200, description="Current Elo rating of this submission")
    matches_played: int = Field(
        0, description="Total number of matches played by this submission"
    )
    created_at: datetime = Field(default_factory=datetime.utcnow)


class Player(BaseModel):
    player_id: str = Field(..., description="Unique ID of the player")
    nickname: str = Field(..., description="Player's display name")
    # KI-Agent unterstützt: Name-claiming identity + hashed recovery code (ADR-0027)
    nickname_normalized: str = Field(
        ..., description="Normalized name (trim+lowercase); unique ownership key"
    )
    recovery_code_hash: Optional[str] = Field(
        None, description="SHA-256 hash of the recovery code; never the plaintext"
    )
    elo_rating: int = Field(1200, description="Aggregate Elo rating of the player")
    matches_played: int = Field(
        0, description="Total number of matches played by the player"
    )
    created_at: datetime = Field(default_factory=datetime.utcnow)


class MatchResult(BaseModel):
    timestamp: datetime = Field(default_factory=datetime.utcnow)
    red_submission_id: str = Field(..., description="MongoDB ID of the red submission")
    blue_submission_id: str = Field(
        ..., description="MongoDB ID of the blue submission"
    )
    winner: str = Field(..., description="Winner: red, blue, or draw")
    red_population: int
    blue_population: int
    generations: int = 100
    elo_delta: int = Field(..., description="Elo change for this match")


class Highlight(BaseModel):
    metric_type: str = Field(
        ..., description="Type of metric (duration, volatility)", examples=["duration"]
    )
    red_name: str
    blue_name: str
    red_seed: List[int] = Field(..., description="8x8 grid as list of 64 integers")
    blue_seed: List[int] = Field(..., description="8x8 grid as list of 64 integers")


class EpochHighlights(BaseModel):
    epoch_id: str
    timestamp: datetime = Field(default_factory=datetime.utcnow)
    highlights: List[Highlight]
