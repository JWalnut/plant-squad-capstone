/* Author: John Walnut
   Purpose:
    Implementation of I2C driver code.
*/

#include "msp430.h"
#include "i2c_driver.h"
#include "salvo.h"
#include "TI_USCI_I2C_master.h"

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
    UCB0CTL1 |= UCTR; //Always start in TX mode

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
    UCB1CTL1 |= UCTR;

    ENABLE_SECONDARY_I2C;

  } else { //Invalid input
    configStruct -> error = I2CERR_UNSPECIFIED_ERR
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

void i2cInitializeMessage(I2CMessage* messageStruct, char* message, int messageLength, char address, char txrxMode, \
                          int respLen, char* response, char i2cInterface) {

  //Error checks
  if (!messageStruct) {//Null pointer check
    return;
  }
  if (!message || !response) { //Null pointer checks
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

  messageStruct -> message = message;
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
  char singleByteRXFlag = 0;

  OSProtect(); //Will this interfere with normal TX/RX interrupt functioning?

  //Error czechs
  if (!messageStruct) { //Null pointer
    OSUnprotect();
    return;
  }
  if (messageStruct->isInitialized != IS_INITIALIZED) { //message has not been initialized'
    messageStruct -> error = I2CERR_STRUCT_NOT_INITIALIZED;
    OSUnprotect();
    return;
  }

  if (messageStruct->messageLength > 0) { //if no TX, then we want to go straight to RX section
    TI_USCI_I2C_transmitinit(messageStruct->address, BAUD_DIVIDE_10);
    while(TI_USCI_I2C_notready());
    if (!TI_USCI_I2C_slave_present(messageStruct->address)) {
      messageStruct -> error = I2CERR_NACK_LIMIT_REACHED;
      return;
    }
    TI_USCI_I2C_transmit(messageStruct->messageLength, messageStruct->message);
  }

  //Receive
  if (messageStruct->txrxMode == RX_MODE) {
    TI_USCI_I2C_receiveinit(messageStruct->address, BAUD_DIVIDE_10);
    while(TI_USCI_I2C_notready());
    TI_USCI_I2C_receive(messageStruct->respLen, messageStruct->response);
  }


  messageStruct -> error = I2CERR_NO_ERROR;
  //OSUnprotect();
  return;
}