#include <__cross_studio_io.h>
#include "salvo.h"
#include "data.h"
#include "tasks.h"

int main(void) {
  OSInit();
  
  WDTCTL = WDTPW + WDTHOLD;
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;


  OSCreateTask(task_getIMUData(), OSTCBP(1), 10);

  while (1) {
    OSSched();

    debug_printf("Gyro (x, y, z): %d%d, %d%d, %d%d\nMagnet (x, y, z): %d%d, %d%d, %d%d\n", 
                  gyroscopeBuffer[X_AXIS_H][bufferIndex], gyroscopeBuffer[X_AXIS_L][bufferIndex],
                  gyroscopeBuffer[Y_AXIS_H][bufferIndex], gyroscopeBuffer[Y_AXIS_L][bufferIndex],
                  gyroscopeBuffer[Z_AXIS_H][bufferIndex], gyroscopeBuffer[Z_AXIS_L][bufferIndex],
                  magnetometerBuffer[X_AXIS_H][bufferIndex], gyroscopeBuffer[X_AXIS_L][bufferIndex],
                  magnetometerBuffer[Y_AXIS_H][bufferIndex], gyroscopeBuffer[Y_AXIS_L][bufferIndex],
                  magnetometerBuffer[Z_AXIS_H][bufferIndex], gyroscopeBuffer[Z_AXIS_L][bufferIndex]);

  }
}

