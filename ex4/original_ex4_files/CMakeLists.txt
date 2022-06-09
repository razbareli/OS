cmake_minimum_required(VERSION 3.20)
project(ex4)

set(CMAKE_CXX_STANDARD 14)

add_executable(ex4
        MemoryConstants.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        SimpleTest.cpp
        VirtualMemory.cpp
        VirtualMemory.h)

add_executable(dan
        MemoryConstants.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        SimpleTest.cpp
        dan.cpp
        VirtualMemory.h)

add_executable(yaara_tests
        YaaraConstants.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        VirtualMemory.cpp
        YaaraTest.cpp
        VirtualMemory.h)

