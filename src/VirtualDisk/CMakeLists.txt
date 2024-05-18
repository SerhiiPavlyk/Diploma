# VirtualEncryptedDiskDriver/CMakeLists.txt

cmake_minimum_required(VERSION 3.21)
project(TESTDriver VERSION 1.0.0)

wdk_add_driver(VirtualEncryptedDiskDriver
    KMDF 1.15
	SRC/IRP/IRP.c
	SRC/main/pch.c
	SRC/main/main.c
	SRC/MountManager/MountManager.c
	SRC/IRP/IRP.h
	SRC/main/pch.h
	SRC/MountManager/MountManager.h
	
	
)


target_include_directories(${PROJECT_NAME} PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/SRC
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)
