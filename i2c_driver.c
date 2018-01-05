/* Author: John Walnut
   Purpose:
    Implementation of I2C driver code.
*/

#include "msp430.h"
#include "i2c_driver.h"
#include "salvo.h"

/* Name: i2cInit
   Description:
    Initializes I2C interface according to settings in parameter configStruct.
*/
void i2cInit(I2CConfig* configStruct) {

  //Error checking
  if (!configStruct) { //Null pointer
    return;
  }
  if (configStruct -> isInitialized != IS_INITIALIZED) { //struct not initialized, continuing would produce undefined behavior
    configStruct -> error = I2CERR_STRUCT_NOT_INITIALIZED;
    return;
  }

  if (configStruct -> i2cInterface == PRIMARY) {

    DISABLE_PRIMARY_I2C; //Disable I2C while we're configuring
    RESET_PRIMARY_CONFIG_0; //Clear config register so we're starting from a clean slate (all zeros)
    UCB0CTL1 = BIT0; //Clear out config register except for reset enable

    UCB0CTL0 |= (UCMST + UCMODE_3 + UCSYNC); //UCMST - master mode, UCMODE_3 - I2C mode, UCSYNC - synchronous mode
    UCB0CTL1 |= (configStruct->clockSource) << CLOCK_SRC_SHIFT; //Double check this...
    PRIMARY_I2C_SEL |= (SCL_PIN + SDA_PIN); //Set pin mode
    UCB0BR0 = ((configStruct->baudDivider) & BAUD_LOW_MASK); //Low 8 bits register
    UCB0BR1 = (configStruct->baudDivider) >> BAUD_SHIFT; //High 8 bits register

    ENABLE_PRIMARY_I2C;

  } else if (configStruct -> i2cInterface == SECONDARY) {

    DISABLE_SECONDARY_I2C;
    RESET_SECONDARY_CONFIG_0;
    UCB1CTL1 = BIT0;

    UCB1CTL0 |= (UCMST + UCMODE_3 + UCSYNC);
    UCB1CTL1 |= (configStruct->clockSource) << CLOCK_SRC_SHIFT;
    SECONDARY_I2C_SEL |= (SCL_PIN + SDA_PIN);
    UCB1BR0 = ((configStruct->baudDivider) & BAUD_LOW_MASK); //Low 8 bits register
    UCB1BR1 = (configStruct->baudDivider) >> BAUD_SHIFT; //High 8 bits register

    ENABLE_SECONDARY_I2C;

  } else { //Invalid input
    configStruct -> error = I2CERR_UNSPECIFIED_ERR;
    return;
  }

  configStruct -> error = I2CERR_NO_ERROR; //All is well
  return;
}

void i2cInitializeConfig(I2CConfig* configStruct, char interface, char clockSource, int baudDivider) {

  //Error checks
  if (!configStruct) { //Null pointer check
    //Don't set error, because will generate segfault
    return;
  }
  if (interface < 0 || interface > 1) { //Range checks
    configStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }
  if (clockSource < 0 || clockSource > 2) {
    configStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }

  configStruct -> i2cInterface = interface;
  configStruct -> clockSource = clockSource;
  configStruct -> baudDivider = baudDivider;

  configStruct -> isInitialized = IS_INITIALIZED;

  configStruct -> error = I2CERR_NO_ERROR;
  return;
}

void i2cInitializeMessage(I2CMessage* messageStruct, char* the_message, int messageLength, char address, char txrxMode, \
                          int respLen, char* response, char i2cInterface) {

  //Error checks
  if (!messageStruct) {//Null pointer check
    return;
  }
  if (!the_message || !response) { //Null pointer checks
    messageStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }
  if (address > MAX_ADDRESS) { //illegal address, other reserved addresses?
    messageStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }
  if (txrxMode < 0 || txrxMode > 1) {
    messageStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }
  if (i2cInterface < 0 || i2cInterface > 1) {
    messageStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }
  if ((txrxMode == RX_MODE) && (respLen <= 0 || response == NO_RESPONSE)) { //Cannot be in receive mode if no place to put data
    messageStruct -> error = I2CERR_BAD_PARAMETERS;
    return;
  }

  messageStruct -> message = the_message;
  messageStruct -> messageLength = messageLength;
  messageStruct -> address = address;
  messageStruct -> txrxMode = txrxMode;
  messageStruct -> respLen = respLen;
  messageStruct -> response = response;
  messageStruct -> i2cInterface = i2cInterface;

  messageStruct -> isInitialized = IS_INITIALIZED;

  messageStruct -> error = I2CERR_NO_ERROR;
  return;
}

void i2cConfigure(I2CConfig* configStruct) {
  i2cInit(configStruct); //This kosher under MISRA?
}

void i2cSendMessage(I2CMessage* messageStruct) { //errors need to be handled

 //Local variables
  char nackCount = 0;
  int i = 0, j = 0;
  char stoppedFlag = 0;

  //OSProtect(); //Will this interfere with normal TX/RX interrupt functioning?

  //Error czechs
  if (!messageStruct) { //Null pointer
    //OSUnprotect();
    return;
  }
  if (messageStruct->isInitialized != IS_INITIALIZED) { //message has not been initialized'
    messageStruct -> error = I2CERR_STRUCT_NOT_INITIALIZED;
    //OSUnprotect();
    return;
  }

  //Lessgo
  if (messageStruct->i2cInterface == PRIMARY) {
    if (!GET_PRIMARY_IS_ACTIVE) { //Interface is not active
      messageStruct -> error = I2CERR_INTERFACE_NOT_ACTIVE;
      //OSUnprotect();
      return; //Must be activated manually
    }
    
    //Initialize
    UCB0I2CSA = messageStruct->address; //Set address

    if (messageStruct->messageLength > 0) { //if no TX, then we want to go straight to RX section
      UCB0CTL1 |= UCTR; //Always start in TX mode
      START_PRIMARY_I2C; //Set start condition
    }

    i = 0;
    while (i < messageStruct->messageLength) {//takes care of RX only case: if message is empty, while loop never run
      if (!GET_PRIMARY_TX_READY) {
        if (PRIMARY_GET_NACK) { //Send same byte again if slave transmitted a NACK
          nackCount++;
	  i = 0; //Starting from the beginning
          if (nackCount >= MAX_NACK) { //Slave unreachable, abort to prevent stalling OS
            STOP_PRIMARY_I2C;
            DISABLE_PRIMARY_I2C; //Problem with the slave, so shut down interface to prevent undefined behavior (not sure what behavior, though)
            messageStruct -> error = I2CERR_NACK_LIMIT_REACHED;
            //OSUnprotect();
            return;
          }
          START_PRIMARY_I2C; //Repeated start
        }
        continue;
      } else {
        UCB0TXBUF = messageStruct->message[i];
        nackCount = 0; //byte got through, slave is reachable.
        i++;
      }
    }

    IFG2 &= ~UCB0TXIFG; //clear TX flag (the datasheet told me to, here in case no RX)

    //Don't think we need this, as bus is held before slave ACK, waiting for action
    //(in this case, setting repeated start/stop)
    //while(!GET_PRIMARY_TX_READY); //wait for last byte to finish sending

    //Receive
    if (messageStruct->txrxMode == RX_MODE) {
      UCB0CTL1 &= ~UCTR; //Set to receive mode
      IFG2 &= ~UCB0TXIFG; //clear TX flag (the datasheet told me to)
      START_PRIMARY_I2C; //(repeated) start condition

      //special case for single byte receive
      if ((messageStruct->respLen == 1) && !PRIMARY_GET_STT_CLR) { //Send stop cond. as soon as STT is cleared if only receiving one byte (from datasheet)
        while(!PRIMARY_GET_STT_CLR);
        STOP_PRIMARY_I2C; //Automatically sends final NACK as required by I2C spec.
        stoppedFlag = 1;
      }

      j = 0;
      nackCount = 0;
      while (j < messageStruct->respLen) {
        if (!GET_PRIMARY_RX_READY) {
          if (PRIMARY_GET_NACK) { //Send same byte again if slave transmitted a NACK
	    nackCount++;
	    if (nackCount >= MAX_NACK) { //Slave unreachable, abort to prevent stalling OS
	      STOP_PRIMARY_I2C;
	      DISABLE_PRIMARY_I2C; //Problem with the slave, so shut down interface to prevent undefined behavior (not sure what behavior, though)
	      messageStruct -> error = I2CERR_NACK_LIMIT_REACHED;
	      //OSUnprotect();
	      return;
	    }
	    START_PRIMARY_I2C; //Repeated start
	  }
	  continue;
        } else {
          messageStruct->response[j] = UCB0RXBUF;
          j++;
        }
      }
      //All done (pretty sure...)
    }

    //Stop communication
    if (!stoppedFlag) { //prevents repeated stop (don't know if that would be a problem)
      STOP_PRIMARY_I2C;
      while(UCB0CTL1 & UCTXSTP); //wait for stop condition to be sent
    }

    messageStruct -> error = I2CERR_NO_ERROR;
    //OSUnprotect();
    return;

  } else if (messageStruct->i2cInterface == SECONDARY) {
    if (!GET_SECONDARY_IS_ACTIVE) { //Interface is not active
      messageStruct -> error = I2CERR_INTERFACE_NOT_ACTIVE;
      //OSUnprotect();
      return; //Must be activated manually
    }
    
    //Initialize
    UCB1I2CSA = messageStruct->address; //Set address

    if (messageStruct->messageLength > 0) {
      UCB1CTL1 |= UCTR;
      START_SECONDARY_I2C; //Set start condition
    }

    //Transmit data
    i = 0;
    while (i < messageStruct->messageLength) {//takes care of RX only case: if message is empty, while loop never run
      if (!GET_SECONDARY_TX_READY) {
        if (SECONDARY_GET_NACK) { //Send same byte again if slave transmitted a NACK
          nackCount++;
	  i = 0;
          if (nackCount >= MAX_NACK) { //Slave unreachable, abort to prevent stalling OS
            STOP_SECONDARY_I2C;
            DISABLE_SECONDARY_I2C; //Problem with the slave, so shut down interface to prevent undefined behavior (not sure what behavior, though)
            messageStruct -> error = I2CERR_NACK_LIMIT_REACHED;
            //OSUnprotect();
            return;
          }
          START_SECONDARY_I2C; //Repeated start
        }
        continue;
      } else {
        UCB1TXBUF = messageStruct->message[i];
        nackCount = 0; //byte got through, slave is reachable.
        i++;
      }
    }

    UC1IFG &= ~UCB1TXIFG; //clear TX flag (the datasheet told me so, here in case no RX)

    //Unnecessary, I think...
    //while(!GET_SECONDARY_TX_READY); //wait for final byte to finish transmission

    //Receive
    if (messageStruct->txrxMode == RX_MODE) {
      UCB1CTL1 &= ~UCTR; //Set to receive mode
      UC1IFG &= ~UCB1TXIFG; //clear TX flag (the datasheet told me so)
      START_SECONDARY_I2C; //(repeated) start condition

      //special case for single byte transmission
      if ((messageStruct->respLen == 1) && !SECONDARY_GET_STT_CLR) { //Send stop cond. as soon as STT is cleared if only receiving one byte (from datasheet)
        while(!SECONDARY_GET_STT_CLR);
        STOP_SECONDARY_I2C; //Automatically sends final NACK as required by I2C spec.
        stoppedFlag = 1;
      }

      j = 0;
      nackCount = 0;
      while (j < messageStruct->respLen) {
        if (!GET_SECONDARY_RX_READY) {
          if (SECONDARY_GET_NACK) { //Send same byte again if slave transmitted a NACK
	    nackCount++;
	    if (nackCount >= MAX_NACK) { //Slave unreachable, abort to prevent stalling OS
	      STOP_SECONDARY_I2C;
	      DISABLE_SECONDARY_I2C; //Problem with the slave, so shut down interface to prevent undefined behavior (not sure what behavior, though)
	      messageStruct -> error = I2CERR_NACK_LIMIT_REACHED;
	      //OSUnprotect();
	      return;
	    }
	    START_SECONDARY_I2C; //Repeated start
	  }
	  continue;
        } else {
          j++;
          if (j >= messageStruct->respLen) {
            STOP_SECONDARY_I2C;
            stoppedFlag = 1;
          }
          messageStruct->response[j] = UCB1RXBUF;
        }
      }
      //All done (pretty sure...)
    }

    //Stop communication
    if (!stoppedFlag) {
      STOP_SECONDARY_I2C;
      while(UCB1CTL1 & UCTXSTP); //wait for stop condition to clear to prevent issues
    }

    messageStruct -> error = I2CERR_NO_ERROR;
    //OSUnprotect();
    return;

  } else {
    messageStruct -> error = I2CERR_UNSPECIFIED_ERR;
    //OSUnprotect();
    return;
  }  
}
