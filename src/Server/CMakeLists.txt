cmake_minimum_required(VERSION 3.10)

project(Server)

# Compiler and flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Source files
set(SRC
    main.cpp
)

# Header files

# Build the target executable
add_executable(main ${SRC})

# Optionally, you can specify the output directory for the executable
# set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Clean up object files
add_custom_target(clean_obj
    COMMAND ${CMAKE_COMMAND} -P clean.cmake
)

## Specify the "all" target
#add_custom_target(all DEPENDS main)

# Clean target
#add_custom_target(clean
#    COMMAND ${CMAKE_BUILD_TOOL} clean_obj
#    COMMAND ${CMAKE_COMMAND} -E remove -f main
#)

# Set the default build target to "all"
#set_directory_properties(PROPERTIES
#    ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/main;${CMAKE_BINARY_DIR}/CMakeCache.txt;${CMAKE_BINARY_DIR}/cmake_install.cmake"
#)
