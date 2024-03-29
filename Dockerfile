FROM gcc:latest as build

WORKDIR /test_build

# Update package list and install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    g++ \
	libpq-dev \
    libpqxx-dev \
    libboost-dev libboost-program-options-dev && \
    rm -rf /var/lib/apt/lists/*
    
# Install nlohmann/json
RUN git clone https://github.com/nlohmann/json.git && \
    mkdir -p json/build && \
    cd json/build && \
    cmake .. && \
    make install && \
    cd ../.. && \
    rm -rf json

# Copy your C++ server source code into the container
ADD ./src /app/src

WORKDIR /app/build

# Build your C++ server
RUN cmake ../src/Server && \
    cmake --build .

# Command to run your server
ENTRYPOINT ["./Server"]
