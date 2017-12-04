/*
  Author: John Walnut
  Hardware Dependencies:
    P5.2 - UCB1-SCL
    P5.1 - UCB1-SDA
    P3.2 - UCB0-SCL (also used for SD card SPI, goes through isolator on MB)
    P3.1 - UCB0-SDA (also used for SD card SPI, goes through isolator on MB)
    P3.0 - -CS_SD/I2C_ON (controls SD card isolator on MB)
  Modifications:
    P5SEL
    P3SEL
    UCB0CTL0
    UCB0CTL1
    UCB1CTL0
    UCB1CTL1
    IE2
    UC1IE
    IFG2
    UC1IFG
    P3DIR
    P3OUT
  Purpose:
    This module is intended to be an API for I2C communication on the MSP430.  It includes function definitions for
    initialization and communication.  It does not contain Salvo-specific functions (semaphores, messages, scheduler commands,
    etc.), but it does assume the CubeSat PPM architecture for the MSP430F2618, including specific GPIO pins, onboard external
h    oscillators, and other necessary settings.
*/

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "msp430.h"


/* DEFINITIONS */

#define SCL_PIN               BIT2 //Same pins for both ports
#define SDA_PIN               BIT1
#define PRIMARY               0 
#define SECONDARY             1
#define TX_MODE               1
#define RX_MODE               0
#define IS_INITIALIZED        0x42 //Used when initializing config struct to indicate successful initialization
#define MAX_ADDRESS           255 //find out what max I2C address is
#define UCLKI                 0x00
#define ACLK                  0x01
#define SMCLK                 0x02
#define BAUD_LOW_MASK         0x00FF
#define CLOCK_SRC_SHIFT       6
#define BAUD_SHIFT            8
#define MAX_NACK              10 //Maximum numbers of NACKs to receive from slave before driver assumes that slave is unreachable and quits
#define NO_RESPONSE           0 //pass to "response" parameter of i2cInitializeMessage when no response is expected (null pointer)
#define NO_MESSAGE            0 //pass to "message" parameter of i2cInitializeMessage when no message is sent (null pointer)
#define BAUD_DIVIDE_10        10

//Primary I2C (on Port 3)
#define PRIMARY_I2C_SEL       P3SEL //This port shares lines with SD card SPI interface and runs through isolator on MB
#define PRIMARY_I2C_DIR       P3DIR //Included for completeness
#define PRIMARY_I2C_OUT       P3OUT //Included to use the isolator pin
#define SD_I2C_ISOL           BIT0 //Activate/deactivate SD card isolator on MB

//Secondary I2C (on Port 5)
#define SECONDARY_I2C_SEL     P5SEL
#define SECONDARY_I2C_DIR     P5DIR


/* MACROS */

//Primary I2C (on Port 3)
#define DISABLE_PRIMARY_I2C       (UCB0CTL1 |= UCSWRST)
#define ENABLE_PRIMARY_I2C        (UCB0CTL1 &= ~UCSWRST)
#define SET_I2C_MODE_PRIMARY      (UCB0CTL0 |= UCMODE_3)
#define SET_SYNC_PRIMARY          (UCB0CTL0 |= USYNC)
#define RESET_PRIMARY_CONFIG_0    (UCB0CTL0 ^= UCB0CTL0)
#define RESET_PRIMARY_CONFIG_1    (UCB0CTL1 ^= UCB0CTL1)
#define GET_PRIMARY_IS_ACTIVE     ~(UCB0CTL1 & UCSWRST) //Returns inverted value of reset pin to indicate active (1) or inactive (0)
#define START_PRIMARY_I2C         (UCB0CTL1 |= UCTXSTT) //generate start condition by setting start bit in config reg
#define PRIMARY_TXRX_INT_EN       (IE2 |= UCB0TXIE + UCB0RXIE)
#define PRIMARY_TXRX_INT_DIS      (IE2 &= ~(UCB0TXIE + UCB0RXIE))
#define GET_PRIMARY_TX_READY      (IFG2 & UCB0TXIFG) //Check flags
#define GET_PRIMARY_RX_READY      (IFG2 & UCB0RXIFG)
#define STOP_PRIMARY_I2C          (UCB0CTL1 |= UCTXSTP) //generate stop condition
#define SET_ISOL_PIN_OUT          (PRIMARY_I2C_DIR |= SD_I2C_ISOL) //Set pin to indicate output
#define ENABLE_ISOL_I2C           (PRIMARY_I2C_OUT |= SD_I2C_ISOL) //Double check to make sure this activates I2C
#define ENABLE_ISOL_SD            (PRIMARY_I2C_OUT &= ~SD_I2C_ISOL) //Do we want these macros in I2C, since it also applies to SPI?
#define PRIMARY_GET_NACK          (UCB0STAT & UCNACKIFG)
#define PRIMARY_GET_STT_CLR       ~(UCB0CTL1 & UCTXSTT) //gets whether start bit has cleared

//Secondary I2C (on Port 5)
#define DISABLE_SECONDARY_I2C     (UCB1CTL1 |= UCSWRST)
#define ENABLE_SECONDARY_I2C      (UCB1CTL1 &= ~UCSWRST)
#define SET_I2C_MODE_SECONDARY    (UCB1CTL0 |= UCMODE_3)
#define SET_SYNC_SECONDARY        (UCB1CTL0 |= USYNC)
#define RESET_SECONDARY_CONFIG_0  (UCB1CTL0 ^= UCB1CTL0)
#define RESET_SECONDARY_CONFIG_1  (UCB1CTL1 ^= UCB1CTL1)
#define GET_SECONDARY_IS_ACTIVE   ~(UCB1CTL1 & UCSWRST)
#define START_SECONDARY_I2C       (UCB1CTL1 |= UCTXSTT)
#define SECONDARY_TXRX_INT_EN     (UC1IE |= (UCB1TXIE + UCB1RXIE))
#define SECONDARY_TXRX_INT_DIS    (UC1IE &= ~(UCB1TXIE + UCB1RXIE))
#define GET_SECONDARY_TX_READY    (UC1IFG & UCB1TXIFG)
#define GET_SECONDARY_RX_READY    (UC1IFG & UCB1RXIFG)
#define STOP_SECONDARY_I2C        (UCB1CTL1 |= UCTXSTP)
#define SECONDARY_GET_NACK        (UCB1STAT & UCNACKIFG)
#define SECONDARY_GET_STT_CLR     ~(UCB1CTL1 & UCTXSTT)



/* DATATYPES */


/* Name: I2CError_e
   Type: enum
   Values:
    I2CERR_STRUCT_NOT_INITIALIZED (0) - Indicates that the structure (either I2CConfig or I2CMessage) passed to the function was not previously
                                    initialized.  That is, its isInitialized parameter does not equal 0x42.
    I2CERR_NACK_LIMIT_REACHED (1) - Indicates that the number of NACK responses defined in MAX_NACK was exceeded.  This may indicate that
                                the peripheral is unreachable.
    I2CERR_INTERFACE_NOT_ACTIVE (2) - Indicates that the i2cSendMessage() was called on an interface which is not active.  Interfaces must be
                                  activated manually through i2cInit().
    I2CERR_NO_ERROR (3) - Indicates no error occurred.  Method returned successfully.
    I2CERR_UNSPECIFIED_ERROR (4) - Indicates that a general error occurred.  No further information is known.
    I2CERR_BAD_PARAMETERS (5) - Indicates that an initialization parameter for the struct was out of bounds, or in some other way illegal.
                                If this error is set, the initialization method returns, and the struct is unchanged (except, of course,
                                for its "error" parameter).
   Purpose: 
    Provides information about the errors thrown by the I2C methods
*/
enum I2CError_e {I2CERR_NO_ERROR = 0,
                 I2CERR_STRUCT_NOT_INITIALIZED = 1,
                 I2CERR_NACK_LIMIT_REACHED = 2,
                 I2CERR_INTERFACE_NOT_ACTIVE = 3,
                 I2CERR_UNSPECIFIED_ERR = 4,
                 I2CERR_BAD_PARAMETERS = 5};
typedef enum I2CError_e I2CError;

/* Name: I2CConfig_s
   Type: struct
   Parameters:
    char i2cInterface - Indicates which of the MSP430F2618's two I2C interfaces should be configured, either primary or secondary.
                        It is recommended to use the predefined PRIMARY and SECONDARY macros
    char clockSource - Selects the MSP430 clock source to the I2C interface, may be fazed out in later versions of the driver.  Values
                       must fall between 0 and 3 inclusive.  For now, always use the predefined SMCLK.
    int baudDivider - Value by which to divide the clock source frequency for I2C.  This may be fazed out in later versions of the driver.
                      For now, always use the predefined BAUD_DIVIDE_10.
    I2CError error - Error message.  Contains information about any errors that occur when this struct is used/initialized.
    char isInitialized - If not set to 0x42, then the struct has not been initialized and anything using it should abort.  Never set
                         directly, but always within driver methods.
   Purpose:
    Contains configuration settings for specified I2C interface.  Used both to initialize the interface, and to
    change configuration settings as needed.
*/
struct I2CConfig_s {
  char i2cInterface;
  char clockSource;
  int baudDivider;
  I2CError error;
  char isInitialized;
};
typedef struct I2CConfig_s I2CConfig;

/* Name: I2CMessage_s
   Type: struct
   Parameters:
    char i2cInterface - Indicates on which of the MSP430F2618's two I2C interfaces the message should be sent, either primary or secondary.
                        It is recommended to use the predefined PRIMARY and SECONDARY macros
    int messageLength - Indicates the length (in bytes) of the message to prevent buffer overflows
    char* message - Pointer to the char array that holds the message to be transmitted
    char address - Address of the I2C peripheral to which to send the message (for possible values, see i2c_peripherals.h)
    char txrxMode - Indicates whether the driver should only send data (TX_MODE) or should expect to receive data (RX_MODE).  It is possible
                    to send and receive data in a single call to i2cSendMessage().  If any data at all is to be received, txrxMode should
                    be set to RX_MODE.
    int respLen - Indicates the length (in bytes) of the response array (that is, the length of the expected response).  This should be exactly
                  the length of the expected response.
    char* response - Pointer to the char array that will hold the response upon its reception from the peripheral
    I2CError error - Error message.  Contains information about any errors that occur when this struct is used/initialized.
    char isInitialized - If not set to 0x42, then the struct has not been initialized and anything using it should abort.  Never set
                         directly, but always within driver methods.
   Purpose:
    Contains all information necessary for an I2C message transaction, including the message, I2C address, interface,
    and space for any return information.
*/
struct I2CMessage_s {
  char i2cInterface;
  int messageLength;
  char* message;
  char address;
  char txrxMode;
  int respLen;
  char* response;
  I2CError error;
  char isInitialized;
};
typedef struct I2CMessage_s I2CMessage;



/* FUNCTION PROTOTYPES */

/* Name: i2cInit
   Parameters:
    I2CConfig* configStruct - configuration structure from which to initialize the I2C channel
   Return value:
    void - error messages are stored in the "error" parameter of configStruct
   Errors:
    I2CERR_STRUCT_NOT_INITIALIZED - Indicates that configStruct was not properly initialized.  No initialization occurs.
    I2CERR_UNSPECIFIED_ERROR - Indicates that an unspecified error occurred.
    I2CERR_NO_ERROR - Initialization successful.
   Description:
    Initializes all port pins and configuration registers for the specified I2C channel, and activates it.  I2C reset pin
    is set at the beginning, which clears TX and RX interrupt enable pins for specified interface.  If there is an error,
    then nothing is changed.  configStruct is not changed, except to set its "error" parameter.
*/
void i2cInit(I2CConfig* configStruct);

/* Name i2cInitializeConfig
   Parameters:
    I2CConfig* configStruct - pointer to the config struct to initialize
    char interface - 0 = primary, 1 = secondary, use defined macros PRIMARY and SECONDARY
    char clockSource - 0 = UCLKI, 1 = ACLK, 2 = SMCLK, use defined macros
    int baudDivider - 16 bit value for clock prescaling
   Return value:
    void - error messages are stored in the "error" parameter of configStruct
   Errors:
    I2CERR_BAD_PARAMETERS - Indicates that one or more parameters passed to the function are out of bounds or otherwise illegal.
                            No initialization occurs.
    I2CERR_NO_ERROR - Initialization successful.
   Description:
    Initializes the configuration struct passed to it.  It is recommended that parameters be passed in the form
    of defined macros.  This function will set the struct's isInitialized variable to 0x42, indicating successful
    initialization.  If there is an error, configStruct will not be changed.
*/
void i2cInitializeConfig(I2CConfig* configStruct, char interface, char clockSource, int baudDivider);

/* Name: i2cInitializeMessage
   Parameters:
    I2CMessage* messageStruct - pointer to structure to initialize
    char* message - pointer to start of outgoing message string
    int length - length of outgoing message (in bytes), if 0, indicates no TX operation, only RX
    char address - value of I2C peripheral address, anything above (tbd) is an illegal address
    int respLen - length of array to store response string (in bytes)
    char* response - character array into which response string is stored
    char i2cInterface - selects I2C interface, either Primary (0) or Secondary (1), predefined macros should be used
   Return value:
    void - error messages are stored in the "error" parameter of messageStruct
   Errors:
    I2CERR_BAD_PARAMETERS - Indicates that one or more parameters passed to the function are out of bounds or otherwise illegal.
                            No initialization occurs.
    I2CERR_NO_ERROR - Initialization successful.
   Description:
    Initializes the message structure passed to it.  It is recommended that parameters be passed, wherever possible,
    in the form of defined macros.  This function will set the struct's isInitialized variable to 0x42, indicating
    successful initialization.  If there is an error, then messageStruct will not be changed.
*/
void i2cInitializeMessage(I2CMessage* messageStruct, char* message, int length, char address, char txrxMode, \
                          int respLen, char* response, char i2cInterface);

/* Name: i2cConfigure
   Parameters:
    I2CConfig* configStruct - pointer to the configuration structure from which to configure the I2C channel
   Return value:
    void - error messages are stored in the "error" parameter of configStructErrors:
   Error:
    I2CERR_STRUCT_NOT_INITIALIZED - Indicates that configStruct was not properly initialized.  No initialization occurs.
    I2CERR_UNSPECIFIED_ERROR - Indicates that an unspecified error occurred.
    I2CERR_NO_ERROR - Initialization successful.
   Description:
    Just a wrapper for i2cInit, used to adjust config settings after the channel has been initialized.
*/
void i2cConfigure(I2CConfig* configStruct);

/* Name: i2cSendMessage
   Parameters:
    I2CMessage* messageStruct - pointer to a structure containing the message to send
   Return value:
    void - error messages are stored in the "error" parameter of messageStruct
   Error:
    I2CERR_STRUCT_NOT_INITIALIZED - Indicates that messageStruct was not properly initialized.
    I2CERR_INTERFACE_NOT_ACTIVE - Indicates that the specified interface is not active.  Interface must be activated manually by using
                                  i2cInit().
    I2CERR_NACK_LIMIT_REACHED - Number of NACKs specified in MAX_NACK was exceeded during transmission.
    I2CERR_UNSPECIFIED_ERROR - Unknown error occurred.
    I2CERR_NO_ERROR - Message successfully sent.
   Description:
    Sends a message over the I2C interface specified in messageStruct and saves response to response.  Any response data
    over length respLen will be ignored.  THIS METHOD IS CRITICAL AND SHOULD BE MARKED AS SUCH (this isdone in the method body).  
    TX and RX interrupts for UCB0 or UCB1 (whichever is specified) are enabled at the beginning of this method, and explicitly disabled at the end.
*/
void i2cSendMessage(I2CMessage* messageStruct);

//currently not needed and not implemented
I2CConfig* i2cGetConfigStruct(char i2cInterface); //do we need this?
int i2cCalculateChecksum(I2CMessage* message); //do we need this?

#endif