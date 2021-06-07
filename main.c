#include "RS485_Wind_Direction_Transmitter.h"


int main()
{
  char Address = 2;
  int WindDirection = 0;
  //char *Direction[16]={"北", "东北偏北", "东北", "东北偏东", "东", "东南偏东", "东南", "东南偏南", "南", "西南偏南", "西南", "西南偏西", "西", "西北偏西", "西北", "西北偏北"};
  char *Direction[16] = {"North", "Northeast by north", "Northeast", "Northeast by east", "East", "Southeast by east", "Southeast", "Southeast by south", "South", "Southwest by south", "Southwest", "Southwest by west", "West", "Northwest by west", "Northwest", "Northwest by north"};
  while (Init("/dev/ttyUSB0")) {
    delay(1000);
  }


  //Modify sensor address
  // if (!ModifyAddress(0, Address)) {
  //   printf("Please check whether the sensor connection is normal\n");
  //   return 0;
  // }


  while (1) {
    WindDirection = readWindDirection(Address);
    if (WindDirection >= 0) {
      printf("WindDirection:%s\n\n", Direction[WindDirection]);
    } else {
      printf("Please check whether the sensor connection is normal\n");
      return 0;
    }
    delay(50);
  }
  serialClose(fd);
  return 1;
}