from pydantic import BaseModel, Field
from typing import List, Tuple

# KI-Agent unterstützt: Pydantic models for Biotope JSON format with detailed documentation


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
