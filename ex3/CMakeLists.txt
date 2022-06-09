#cmake_minimum_required(VERSION 3.20)
#project(ex3)
#
#set(CMAKE_CXX_STANDARD 14)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#
#
#add_executable(ex3 main.cpp)
#add_executable(sample_client Barrier/Barrier.h Barrier/Barrier.cpp MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp SampleClient/SampleClient.cpp)
#
#add_executable(git Barrier/Barrier.h MapReduceFramework.h MapReduceClient.h from_git.cpp SampleClient/SampleClient.cpp)
#add_executable(barrier Barrier/Barrier.h Barrier/Barrier.cpp Barrier/barrierdemo.cpp)
#add_executable(atomicDemo Atomic/atomicdemo.cpp)
#add_executable(atomicMulti Atomic/atomicdemo_multivar.cpp)


cmake_minimum_required(VERSION 3.1)


# NOTE: You can't have both ThreadSanitizer and AddressSanitizer enabled at the same time.

# Uncomment the following to enable ThreadSanitizer.
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=thread")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=thread")

# Uncomment the following to enable AddressSanitizer.
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")


# Project configuration
project(ex3 VERSION 1.0 LANGUAGES C CXX)


# Ensure system has pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
find_package(Threads REQUIRED)

add_executable(sample_client Barrier/Barrier.h Barrier/Barrier.cpp MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp SampleClient/SampleClient.cpp)
add_executable(git Barrier/Barrier.h MapReduceFramework.h MapReduceClient.h from_git.cpp SampleClient/SampleClient.cpp)
add_executable(test1 Barrier/Barrier.h Barrier/Barrier.cpp MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp test1.cpp)
add_executable(test2 Barrier/Barrier.h Barrier/Barrier.cpp MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp test2.cpp)
add_executable(test3 Barrier/Barrier.h Barrier/Barrier.cpp MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp test3.cpp)
add_executable(test4 Barrier/Barrier.h Barrier/Barrier.cpp MapReduceFramework.h MapReduceClient.h MapReduceFramework.cpp test4.cpp)




add_library(MapReduceFramework
        MapReduceClient.h
        MapReduceFramework.cpp MapReduceFramework.h
        Barrier/Barrier.h Barrier/Barrier.cpp
        # ------------- Add your own .h/.cpp files here -------------------
        )


set_property(TARGET MapReduceFramework PROPERTY CXX_STANDARD 11)
target_compile_options(MapReduceFramework PUBLIC -Wall -Wextra)
target_include_directories(MapReduceFramework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# link pthreads to your framework
target_link_libraries(MapReduceFramework PUBLIC Threads::Threads)

# Add tests
add_subdirectory(mattanTests)

