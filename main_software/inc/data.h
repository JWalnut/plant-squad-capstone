/* Author: John Walnut
   Purpose: Defines data structures for manipulation of data.
*/

#ifndef DATA_H
#define DATA_H

/* CONSTANTS */
#define X_AXIS_H 0
#define X_AXIS_L 1
#define Y_AXIS_H 2
#define Y_AXIS_L 3
#define Z_AXIS_H 4
#define Z_AXIS_L 5

#define IMU_DATA_RESP_LEN 6
#define IMU_DATA_MSG_LEN 1

/* VARIABLES */
int gyroscopeBuffer[6][100]; //First dimension for three axes, use predefined "X_AXIS", etc.
int magnetometerBuffer[6][100];
int bufferIndex;

/* DATATYPES */

/* Name: RadioHealth
   Type: struct
   Parameters:
     TBD
   Purpose:
     To keep track of the current health of RFM radio.
*/
struct RadioHealth_s {
  //things
};
typedef RadioHealth struct RadioHealth_s;

/* Name: IMUHealth
   Type: struct
   Parameters:
     TBD
   Purpose:
     To keep track of current health of IMU.
*/
struct IMUHealth_s {
  //things
};
typedef IMUHealth struct IMUHealth_s;