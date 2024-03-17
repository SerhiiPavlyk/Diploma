cmake_minimum_required(VERSION 3.2)

project(Server)

# Compiler and flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(Boost_USE_STATIC_LIBS ON)
# find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${BOOST_INCLUDE_DIRS})

add_executable(Server
main.cpp pch.cpp PostDataParser.cpp RequestHandler.cpp Server.cpp
		 pch.h 	 PostDataParser.h   RequestHandler.h   Server.h
)


#target_link_libraries(Server ${Boost_LIBRARIES})