/* Author: John Walnut 
   Purpose: Defines Salvo tasks for software
*/

#include "salvo.h"
#include "msp430.h"

#ifndef TASKS_H
#define TASKS_H

/* TASK PROTOTYPES */

/* Name: task_getIMUData
   Purpose: Gets data from IMU over I2C bus.  This includes gyroscope and magnetometer data (?).
*/
void task_getIMUData();

/* Name: task_kalmanFilter
   Purpose: Runs Kalman filter on retrieved IMU data.  Don't know if this is strictly necessary.
*/
void task_kalmanFilter();

/* Name: task_getHealth
   Purpose: Gest health data from radio and IMU and saves to a health struct.
*/
void task_getHealth();

/* Name: task_sendData
   Purpose: Sends data (both Kalman filter results, and health data if available) through RFM radio to counterpart.
*/
void task_sendData();

/* SALVO DEFINITIONS */
#define TASK_GET_IMU_DATA OSTCBP(1)
#define TASK_RUN_KALMAN_FILTER OSTCBP(2)
#define TASK_GET_HEALTH_INFO OSTCBP(3)
#define TASK_SEND_DATA OSTCBP(4)

#endif
