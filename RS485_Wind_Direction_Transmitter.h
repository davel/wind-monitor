#ifndef __RS485_Wind_Direction_Transmitter_H__
#define __RS485_Wind_Direction_Transmitter_H__

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <wiringPi.h>

#include <wiringSerial.h>

/**
  @brief  Add CRC parity bit
  @param  buf Data to which parity bits are to be added
  @param  len Check the length of the data.
*/
extern void addedCRC(unsigned char *buf, int len);

/**
  @brief  Read wind direction
  @param  Address The address where you want to read the data
  @return  The return value ≥0 indicates successful reading, the return value is the wind direction corresponding to the following table, and the return value is -1 indicates failed reading
*/
double readWindDirection(unsigned char Address);


/**
  @brief  Calculate parity bit
  @param  buf Data to be verified
  @param  len Data length
  @return  The return value is the calculated parity bit
*/
// unsigned int CRC16_2(unsigned char *buf, int len);

#endif
