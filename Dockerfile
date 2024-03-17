FROM gcc:latest as build

# Update package list and install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    g++ \
    libboost-dev libboost-program-options-dev && \
    rm -rf /var/lib/apt/lists/*
    
# Build your C++ server

RUN ls -l

WORKDIR ./src

RUN ls -l

RUN cmake -S ./Server -B ./build && \ 
    cmake --build ./build

WORKDIR ./build

RUN ls -l

# Command to run your server
ENTRYPOINT ["./Server"]
