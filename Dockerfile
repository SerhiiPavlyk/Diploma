# =========================
# Stage 1: Build the server
# =========================
FROM ubuntu:24.04 AS build

# Set noninteractive mode for apt
ENV DEBIAN_FRONTEND=noninteractive

# Update packages and install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    g++ \
    libpq-dev \
    libpqxx-dev \
    libboost-dev \
    libboost-program-options-dev \
    && rm -rf /var/lib/apt/lists/*

# Install nlohmann/json from source
RUN git clone https://github.com/nlohmann/json.git /tmp/json && \
    cmake -S /tmp/json -B /tmp/json/build && \
    cmake --build /tmp/json/build --target install && \
    rm -rf /tmp/json

# Copy your source code into the image
WORKDIR /app
COPY ./src ./src

# Create a build directory and compile your C++ server
RUN mkdir -p build && cd build && \
    cmake ../src/Server && \
    cmake --build . -- -j$(nproc)

# =========================
# Stage 2: Runtime image
# =========================
FROM ubuntu:24.04

# Install only runtime dependencies
RUN apt-get update && apt-get install -y \
    libpq-dev \
    libboost-program-options-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the compiled binary from the build stage
WORKDIR /app
COPY --from=build /app/build/Server .

# Expose port (optional — change if needed)
EXPOSE 8080

# Set the default command
ENTRYPOINT ["./Server"]
