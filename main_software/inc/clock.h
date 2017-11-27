/* Author:
   Purpose: To define everything needed for clocking - both hardware and software
*/

#include "salvo.h"
#include "msp430.h"

#ifndef CLOCK_H
#define CLOCK_H

/* FUNCTION PROTOTYPES */

/* Name: InitializeClock
   Parameters:
     char divider - value by which to divide the master clock into SMCLK
   Purpose: 
     To initialize the master hardware clock.
*/
void InitializeClock(char divider);

/* Name: clockTick
   Purpose:
     Ticks the software clock once - to occur on interrupt
*/
void clockTick();

#endif