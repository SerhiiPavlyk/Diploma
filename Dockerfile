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
    
# Build your C++ server

ADD ./src /app/src

WORKDIR /app/build

RUN cmake ../src/Server && \ 
    cmake --build .

# Command to run your server
ENTRYPOINT ["./Server"]
