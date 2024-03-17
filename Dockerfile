FROM gcc:latest as build

WORKDIR /test_build

RUN echo "Current working directory: $(pwd)"

# Update package list and install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    g++ \
    libboost-dev libboost-program-options-dev && \
    rm -rf /var/lib/apt/lists/*
    
# Build your C++ server

RUN ls -a

RUN echo "Current working directory: $(pwd)"

ADD ./src /app/src

RUN echo "Current working directory: $(pwd)"

RUN ls -a

WORKDIR /app/build

RUN echo "Current working directory: $(pwd)"

RUN ls -l

RUN cmake ../src/Server && \ 
    cmake --build .

WORKDIR /app

RUN echo "Current working directory: $(pwd)"

RUN ls -l

# Command to run your server
ENTRYPOINT ["/app/build/Server/Server"]
