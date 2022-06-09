#include <iostream>
#include "uthreads.h"

void test_1 () {
  bool flag = true;
  std::cout << "Starting thread 1" << std::endl;
  for(;;) {
    std::cout << "im from test_1, my quantums are:" << uthread_get_quantums(1)<<std::endl;
    for(int i = 1; i < 500000000; i++) {
    }
    if (flag) {
      uthread_block(2);
      flag = false;
    }
  }
}
void test_2 () {
  std::cout << "Starting thread 2" << std::endl;
  for(;;) {
    std::cout << "im from test_2, my quantums are:" << uthread_get_quantums(2)<<std::endl;
    for(int i = 1; i < 500000000; i++) {
    }
  }
}

int main() {
  std::cout << "Starting main thread" << std::endl;
  uthread_init(1000000);
  int k = 1;
  for(;;) {
    std::cout << "im from main, my quantums are:" << uthread_get_quantums(0)<<std::endl;
    //        uthread_spawn(&test_1);
    for(int i = 1; i < 1000000000; i++) {
      if (i % 1000000 == 0) {
        switch(k) {
          case 1:
            uthread_spawn(&test_1);
            k++;
            break;
            case 2:
              uthread_spawn(test_2);
              k++;
              break;
              case 3:
                break;
        }
      }
    }
    uthread_resume(2);
//            uthread_terminate(1);
  }
}