FROM gcc:latest

WORKDIR /app

# Install dependencies for Raylib (Linux/X11) and build tools
# KI-Agent unterstützt
RUN apt-get update && apt-get install -y \
    make \
    git \
    cmake \
    libasound2-dev \
    libx11-dev \
    libxrandr-dev \
    libxi-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxcursor-dev \
    libxinerama-dev \
    libwayland-dev \
    libxkbcommon-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone, Build, and Install Raylib from source
# KI-Agent unterstützt
RUN git clone https://github.com/raysan5/raylib.git /tmp/raylib \
    && cd /tmp/raylib/src \
    && make PLATFORM=PLATFORM_DESKTOP \
    && make install \
    && cd / \
    && rm -rf /tmp/raylib

COPY . .

# Automatically start bash
CMD ["bash"]