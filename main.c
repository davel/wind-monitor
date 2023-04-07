// vim:ts=2:shiftwidth=2:expandtab

#include <math.h>
#include <stdio.h>

#include "RS485_Wind_Direction_Transmitter.h"
#include "RS485_Wind_Speed_Transmitter.h"

int fd;

static unsigned char Init(char *device)
{
  if ((fd = serialOpen(device, 9600)) < 0) {
    fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
    return 1;
  }
  return 0;
}

void addedCRC(unsigned char *buf, int len)
{
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
    crc ^= (unsigned int)buf[pos];
    for (int i = 8; i != 0; i--)
    {
      if ((crc & 0x0001) != 0)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  buf[len] = crc % 0x100;
  buf[len + 1] = crc / 0x100;
}


static unsigned char  __attribute__((unused)) ModifyAddress(unsigned char Address1, unsigned char Address2)
{
  unsigned char ModifyAddressCOM[11] = {0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00};
  char ret = 0;
  long curr = millis();
  long curr1 = curr;
  char ch = 0;
  ModifyAddressCOM[0] = Address1;
  ModifyAddressCOM[8] = Address2;
  addedCRC(ModifyAddressCOM , 9);
  write(fd, ModifyAddressCOM, 11);
  while (!ret) {
    if (millis() - curr > 1000) {
      break;
    }

    if (millis() - curr1 > 100) {
      write(fd, ModifyAddressCOM, 11);
      curr1 = millis();
    }

    if (serialDataAvail(fd) > 0) {
      delay(7);
      if (read(fd, &ch, 1) == 1) {
        if (ch == Address1) {
          if (read(fd, &ch, 1) == 1) {
            if (ch == 0x10 ) {
              if (read(fd, &ch, 1) == 1) {
                if (ch == 0x10) {
                  if (read(fd, &ch, 1) == 1) {
                    if (ch == 0x00) {
                      if (read(fd, &ch, 1) == 1) {
                        if (ch == 0x00) {
                          if (read(fd, &ch, 1) == 1) {
                            if (ch == 0x01) {
                              printf("Please re-power the sensor and enter: Y.\n");
                              scanf("%s", &ch);
                              ret = 1 ;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return ret;
}



int main()
{
  const char DirectionAddress = 2;
  const char SpeedAddress     = 4;

  while (Init("/dev/ttyUSB0")) {
    delay(1000);
  }


  //Modify sensor address
  // if (!ModifyAddress(0, Address)) {
  //   printf("Please check whether the sensor connection is normal\n");
  //   return 0;
  // }

  const int points = 40;


  while (1) {
    double WindDirection = -1;
    double WindSin = 0;
    double WindCos = 0;

    double WindSpeed = -1;
    double WindMin = -1;
    double WindMax = -1;
    double WindTotal = 0;

    for (int dataPoints = 0; dataPoints < points; dataPoints++) {
      WindDirection = readWindDirection(DirectionAddress);
      if (WindDirection < 0) {
        printf("Please check whether the sensor connection is normal\n");
        return 0;
      }

      WindSin += sin(WindDirection * (M_PI / 180.0));
      WindCos += cos(WindDirection * (M_PI / 180.0));

      WindSpeed = readWindSpeed(SpeedAddress);
      if (WindSpeed < 0) {
        printf("Please check whether the sensor connection is normal\n");
        return 0;
      }

      if (WindMin == -1 || WindSpeed < WindSpeed) {
        WindMin = WindSpeed;
      }
      if (WindMax == -1 || WindSpeed > WindMax) {
        WindMax = WindSpeed;
      }
      WindTotal += WindSpeed;
    }

    double WindAvg = WindTotal / (double) points;
    double WindDirectionAvg = atan2(WindSin, WindCos) * (180.0 / M_PI);

    printf("Dir %f AvgDir %f Speed %f AvgSpeed %f MaxSpeed %f MinSpeed %f\n",
      WindDirection,
      WindDirectionAvg,
      WindSpeed,
      WindAvg,
      WindMax,
      WindMin
    );

    delay(50);
  }
  serialClose(fd);
  return 1;
}
