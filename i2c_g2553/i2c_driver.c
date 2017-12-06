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
    PRIMARY_I2C_SEL_2 |= (SCL_PIN + SDA_PIN);
    UCB0BR0 = ((configStruct->baudDivider) & BAUD_LOW_MASK); //Low 8 bits register
    UCB0BR1 = (configStruct->baudDivider) >> BAUD_SHIFT; //High 8 bits register

    ENABLE_PRIMARY_I2C;

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
  int i = 0;
  char stoppedFlag = 0;

  //OSProtect(); //Will this interfere with normal TX/RX interrupt functioning?

  //Error czechs
  if (!messageStruct) { //Null pointer
    ////OSUnprotect();
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

    while(!GET_PRIMARY_TX_READY); //wait for last byte to finish sending

    //Receive
    if (messageStruct->txrxMode == RX_MODE) {
      int j = 0;
      UCB0CTL1 &= ~UCTR; //Set to receive mode
      START_PRIMARY_I2C; //(repeated) start condition
      IFG2 &= ~UCB0TXIFG; //clear TX flag (the datasheet told me so)

      //special case for single byte receive
      if ((messageStruct->respLen == 1) && PRIMARY_GET_STT_CLR) { //Send stop cond. as soon as STT is cleared if only receiving one byte (from datasheet)
        while(UCB0CTL1 & UCTXSTT);
        STOP_PRIMARY_I2C; //Automatically sends final NACK as required by I2C spec.
        stoppedFlag = 1;
      }

      while (j < messageStruct->respLen) {
        if (!GET_PRIMARY_RX_READY) {
          if (PRIMARY_GET_NACK) { //Didn't acknowledge address (only has possibility of happening when while loop is entered
            START_PRIMARY_I2C; //Repeated start
            continue;
          }
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
