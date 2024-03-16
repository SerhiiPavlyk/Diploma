# Compiler
CC := g++

# Files
HEADERS := $(wildcard *.h)
SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

# Flags
CPPFLAGS := -I
CXXFLAGS := -Wall -Wextra -std=c++11
LDFLAGS := -lboost_system -lboost_thread

# Target
TARGET := server

# Target Rule
$(TARGET): $(OBJS)
    $(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Phony target
.PHONY: clean

# Clean Rule
clean:
    rm -f $(OBJS) $(TARGET)