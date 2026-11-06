/*
  Name:    setCurrent.ino
  Created: 19-08-2018
  Author:  SolidGeek
  Description: This is a very simple example of how to set the current for the motor
*/

#include <VescUart.h>

/** Initiate VescUart class */
VescUart UART;

float current = 1.0; /** The current in amps */
float rampRate = 10.0; /** Maximum current change in amps per second */

void setup() {
  Serial.begin(9600);
  /** Setup UART port (Serial1 on Atmega32u4) */
  Serial1.begin(19200);
  
  while (!Serial1) {;}

  /** Define which ports to use as UART */
  UART.setSerialPort(&Serial1);
}

void loop() {
  
  /** Call the function setCurrentRamp() periodically to ramp the motor current */
  UART.setCurrentRamp(current, rampRate);

  // UART.setBrakeCurrent(current);
  
}