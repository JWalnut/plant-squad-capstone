/* Author: John Walnut
   Purpose: Defines data structures for manipulation of data.
*/

#ifndef DATA_H
#define DATA_H

/* CONSTANTS */
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

/* VARIABLES */
char gyroscopeBuffer[3][100]; //First dimension for three axes, use predefined "X_AXIS", etc.
int magnetometerBuffer[3][100];

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
