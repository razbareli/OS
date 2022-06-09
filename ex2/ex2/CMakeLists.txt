cmake_minimum_required(VERSION 3.20)
project(ex2)

set(CMAKE_CXX_STANDARD 14)

add_executable(ex2 main.cpp uthreads.cpp uthreads.h)
add_executable(itimer demo_itimer.c)
add_executable(jmp demo_jmp.c)
add_executable(sigint_handler demo_singInt_handler.c)
add_executable(test1 uthreads.cpp uthreads.h tests/test1.in.cpp)
add_executable(test2 uthreads.cpp uthreads.h tests/test2.in.cpp)
add_executable(test3 uthreads.cpp uthreads.h tests/test3.in.cpp)
add_executable(test5 uthreads.cpp uthreads.h tests/test5_no_out.cpp)
add_executable(test6 uthreads.cpp uthreads.h tests/test6_no_out.cpp)
add_executable(test7 uthreads.cpp uthreads.h tests/test7_no_out_different.cpp)
add_executable(jona1 uthreads.cpp uthreads.h tests/jona1.cpp)
add_executable(jona2 uthreads.cpp uthreads.h tests/jona2.cpp)
add_executable(jona3 uthreads.cpp uthreads.h tests/jona3.cpp)
add_executable(jona4 uthreads.cpp uthreads.h tests/jona4.cpp)
add_executable(jona5 uthreads.cpp uthreads.h tests/jona5.cpp)
add_executable(jona6 uthreads.cpp uthreads.h tests/jona6mask_sigvt.cpp)
add_executable(yechiel uthreads.cpp uthreads.h tests/yechiel.cpp)
add_executable(test44 uthreads.cpp uthreads.h tests/test4.in.cpp)
add_executable(yuval uthreads.cpp uthreads.h tests/test_yuval.cpp)
add_executable(iris uthreads.cpp uthreads.h tests/iris.cpp)
add_executable(basic_tests uthreads.cpp uthreads.h tests/basic_test.cpp)








