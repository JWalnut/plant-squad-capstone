#include <__cross_studio_io.h>
#include "salvo.h"

void TaskB(void) {
  while(1) {
    debug_printf("TaskB\n");
    OS_Yield();
  }
}

void TaskA(void) {
  while(1) {
    debug_printf("TaskA\n");
    OS_Yield();
  }
}


int main(void) {
  OSInit();

  OSCreateTask(TaskA, OSTCBP(1), 10);
  OSCreateTask(TaskB, OSTCBP(2), 10);

  while (1) {
    OSSched();
  }
}

