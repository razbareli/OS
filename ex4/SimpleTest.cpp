#include "VirtualMemory.h"
#define MASK ((1LL << OFFSET_WIDTH) -1)



#include <cstdio>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
      VMinitialize();
      std::cout<<NUM_FRAMES<<std::endl;
      for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
          printf("writing to %llu\n", (long long int) i);
          VMwrite(5 * i * PAGE_SIZE, i);
      }

      for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
          word_t value;
          VMread(5 * i * PAGE_SIZE, &value);
          printf("reading from %llu %d\n", (long long int) i, value);
          assert(uint64_t(value) == i);
      }

  return 0;
}

//int main(int argc, char **argv) {
//    VMinitialize();
//    for (uint64_t i = 0; i < 2; ++i) {
//        printf("writing to %llu\n", (long long int) i);
//        VMwrite(5 * 1 * PAGE_SIZE, 99);
//    }
//
//    for (uint64_t i = 0; i < 2; ++i) {
//        word_t value;
//        VMread(5 * i * PAGE_SIZE, &value);
//        printf("reading from %llu %d\n", (long long int) i, value);
////        assert(uint64_t(value) == i);
//    }
//
//    return 0;
//}

//int main(int argc, char **argv) {
//  VMinitialize();
//  std::cout<<"Table Depth = " << TABLES_DEPTH<<std::endl;
//  std::cout<<"Num of Frames = " << NUM_FRAMES <<std::endl;
//  std::cout<<"Num of Pages = " << NUM_PAGES <<std::endl;
//  int val_to_write = 100;
//  int address = 377154;
//  std::cout <<"Writing " << val_to_write <<" to address "<< address << std::endl;
//  VMwrite (address, val_to_write);
//  word_t val;
//  VMread(377154, &val);
//  std::cout<<"reading from address " << address <<", got the value "<<val<<std::endl;
//}