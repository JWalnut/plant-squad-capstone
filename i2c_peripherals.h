/* Author: John Walnut
   Hardware Dependencies:
    None
   Modifications:
    None
   Purpose:
    A separate file to hold definitions for the I2C peripherals that we will be using, including specific command codes,
    register addresses, and device addresses.
*/
    

#ifndef I2C_PERIPHERALS_H
#define I2C_PERIPHERALS_H

//Peripheral Addresses
#define ANTENNA_I2C_ADDR      0x33
#define IMU_I2C_ADDR          0x69 //on GPS board, AD0 pin is connected to VDDIO
#define EPS_I2C_ADDR          0x2B
#define MAGNET_I2C_ADDR       0x0C

/* Peripheral-specific commands */

//IMU (MPU-9250)
#define WHOAMI_REG            117 //should return 0x71
#define MAGNET_ID_REG         0x00 //should return 0x48

#endif
