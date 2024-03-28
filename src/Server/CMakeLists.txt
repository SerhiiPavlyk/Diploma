include(FetchContent)

cmake_minimum_required(VERSION 3.2)

project(Server)

# Compiler and flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(${BOOST_INCLUDE_DIRS})

FetchContent_Declare(
  libpqxx
  GIT_REPOSITORY https://github.com/jtv/libpqxx.git
  GIT_TAG a6b1d60e74c1427c8ac2324b85cd4a0dc2068332
)
set(PQXX_LIBRARIES pqxx_static)

FetchContent_MakeAvailable(libpqxx)

add_executable(Server
main.cpp pch.cpp PostDataParser.cpp RequestHandler.cpp Server.cpp	DataBase.cpp
		 pch.h 	 PostDataParser.h   RequestHandler.h   Server.h 	DataBase.h
		 
)


target_link_libraries(Server ${Boost_LIBRARIES})
target_link_libraries(Server ${PQXX_LIBRARIES})