from pydantic import BaseModel, Field
from typing import List, Tuple
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


class Submission(BaseModel):
    metadata: Metadata
    config: Config


class DBSubmission(Submission):
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
