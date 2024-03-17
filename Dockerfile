FROM gcc:latest as build

# Update package list and install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    g++ \
    libboost-dev libboost-program-options-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /test_build

RUN echo "Current working directory: $(pwd)"


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

COPY --from=build /app/build/Server .

RUN echo "Current working directory: $(pwd)"

RUN ls -l

# Command to run your server
ENTRYPOINT ["./Server"]
