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
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Clone, Build, and Install Raylib from source for Desktop
# KI-Agent unterstützt
RUN git clone https://github.com/raysan5/raylib.git /tmp/raylib \
    && cd /tmp/raylib/src \
    && make PLATFORM=PLATFORM_DESKTOP \
    && make install \
    && cd / \
    && rm -rf /tmp/raylib

# Install Emscripten SDK
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk \
    && cd /opt/emsdk \
    && ./emsdk install latest \
    && ./emsdk activate latest

# Build Raylib for Web (WASM)
RUN /bin/bash -c "source /opt/emsdk/emsdk_env.sh && \
    git clone https://github.com/raysan5/raylib.git /tmp/raylib_web && \
    cd /tmp/raylib_web/src && \
    make PLATFORM=PLATFORM_WEB -B && \
    mkdir -p /opt/raylib_web/lib && \
    mkdir -p /opt/raylib_web/include && \
    cp libraylib.web.a /opt/raylib_web/lib/libraylib.a && \
    cp raylib.h rlgl.h raymath.h /opt/raylib_web/include/ && \
    cd / && rm -rf /tmp/raylib_web"

# Add emsdk to bashrc so it's available when we open a shell
RUN echo 'source /opt/emsdk/emsdk_env.sh' >> /root/.bashrc

COPY . .

# Automatically start bash
CMD ["bash"]
