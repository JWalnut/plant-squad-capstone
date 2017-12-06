/* Author: John Walnut
   Purpose: Implements Salvo tasks defined in tasks.h
*/

#include "tasks.h"
#include "i2c_driver.h"
#include "i2c_peripherals.h"
#include "data.h"

void task_getIMUData() {
  while(1) {
    I2CConfig cfg;
    I2CMessage msg;

    char message_str[IMU_DATA_MSG_LEN];
    char response_str[IMU_DATA_RESP_LEN];
    char i = 0;

    for (i = 0; i < 2; i++) {

      char j;
      for (j = 0; j < IMU_DATA_RESP_LEN, j++) {
            response_str[i] = 0;
      }

      switch(i) {
        case 0: //magnetometer

          message_str[0] = MAGNET_START;

          i2cInitializeConfig(cfg, SECONDARY, SMCLK, BAUD_DIVIDE_10);
          i2cInitializeMessage(msg, message_str, IMU_DATA_MSG_LEN, MAGNET_ADDR, RX_MODE, IMU_DATA_RESP_LEN, response_str, SECONDARY);
          i2cInit(cfg);
          i2cSendMessage(msg);

          if (bufferIndex == 99) {
            bufferIndex = 0;
          } else {
            bufferIndex++;
          }

          for (j = 0; j < IMU_DATA_RESP_LEN; j++) {
            magnetometerBuffer[j][bufferIndex] = response_str[j];
          }
          break;
        case 1: //gyroscope
    
          message_str[0] = GYROSCOPE_START;

          i2cInitializeConfig(cfg, SECONDARY, SMCLK, BAUD_DIVIDE_10);
          i2cInitializeMessage(msg, message_str, IMU_DATA_MSG_LEN, IMU_I2C_ADDR, RX_MODE, IMU_DATA_RESP_LEN, response_str, SECONDARY);
          i2cInit(cfg);
          i2cSendMessage(msg);

          if (bufferIndex == 99) {
            bufferIndex = 0;
          } else {
            bufferIndex++;
          }

          for (j = 0; j < IMU_DATA_RESP_LEN; j++) {
            gyroscopeBuffer[j][bufferIndex] = response_str[j];
          }
          break;
        default:
          //do nothing
      }
    }
    OS_Yield();
  }
}