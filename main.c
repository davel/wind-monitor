// vim:ts=2:shiftwidth=2:expandtab

#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <curl/curl.h>

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
  CURL *curl;

  const char DirectionAddress = 2;
  const char SpeedAddress     = 4;

  const char *mqtt_username = getenv("MQTT_USERNAME");
  if (!mqtt_username) {
    fprintf(stderr, "No MQTT_USERNAME\n");
    exit(1);
  }

  const char *mqtt_password = getenv("MQTT_PASSWORD");
  if (!mqtt_password) {
    fprintf(stderr, "No MQTT_PASSWORD\n");
    exit(1);
  }

  const char *mqtt_client   = getenv("MQTT_CLIENT");
  if (!mqtt_client) {
    fprintf(stderr, "No MQTT_CLIENT\n");
    exit(1);
  }
  char *url = NULL;
  asprintf(&url, "https://api.mydevices.com/things/%s/data", mqtt_client);

  while (Init("/dev/ttyUSB0")) {
    delay(1000);
  }

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if (!curl) {
    fprintf(stderr, "libcurl could not start.\n");
    return 1;
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

      if ((WindMin == -1) || (WindSpeed < WindMin)) {
        WindMin = WindSpeed;
      }
      if ((WindMax == -1) || (WindSpeed > WindMax)) {
        WindMax = WindSpeed;
      }
      WindTotal += WindSpeed;
    }

    double WindAvg = WindTotal / (double) points;
    double WindDirectionAvg = atan2(WindSin, WindCos) * (180.0 / M_PI);

    if (WindDirectionAvg < 0.0) {
      WindDirectionAvg += 180.0;
    }

    char *json = NULL;

    asprintf(
      &json,
      "["
      "{\"channel\":1,\"value\":%f,\"type\":\"wind_direction\",\"unit\":\"deg\"},"
      "{\"channel\":2,\"value\":%f,\"type\":\"wind_direction\",\"unit\":\"deg\"},"
      "{\"channel\":3,\"value\":%f,\"type\":\"wind_speed\",\"unit\":\"kmh\"},"
      "{\"channel\":4,\"value\":%f,\"type\":\"wind_speed\",\"unit\":\"kmh\"},"
      "{\"channel\":5,\"value\":%f,\"type\":\"wind_speed\",\"unit\":\"kmh\"},"
      "{\"channel\":6,\"value\":%f,\"type\":\"wind_speed\",\"unit\":\"kmh\"}"
      "]",
      WindDirection,
      WindDirectionAvg,
      WindSpeed * 3.6,
      WindAvg   * 3.6,
      WindMin   * 3.6,
      WindMax   * 3.6
    );

    if (getenv("DEBUG")) {
      printf("%s\n", json);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

    struct curl_slist *list = NULL;

    list = curl_slist_append(list, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_USERNAME, mqtt_username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, mqtt_password);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(list);
    free(json);

    if (res != CURLE_OK) {
      fprintf(stderr, "Send failed: %s\n", curl_easy_strerror(res));
    }

    delay(100);
  }
  serialClose(fd);
  return 1;
}
