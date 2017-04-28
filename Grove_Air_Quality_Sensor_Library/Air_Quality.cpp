//
// modified by mbed group for use with the mbed platform
// modification date 9/4/2014
//

/*
  AirQuality library v1.0
  2010 Copyright (c) Seeed Technology Inc.  All right reserved.

  Original Author: Bruce.Qin

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include"Air_Quality.h"

// Interrupt Handler Assignment
Ticker IntHandler;

#if 0
#define PRINTF printf
#else
#define PRINTF(...)
#endif
//#endif

AirQuality::AirQuality(PinName anPin)
    :_sensor(anPin), _pin(anPin){
}

//Get the avg voltage in 5 minutes.
void AirQuality::avgVoltage()
{
    if(i==150) { //sum 5 minutes - 150, 1 min 30(60/2)
        vol_standard=temp/150;
        temp=0;
        i=0;
        if(!aqRefVal) aqRefVal = (int)vol_standard;
        PRINTF("Vol_standard in 2 minutes: %d\n\r",vol_standard);
        wait_ms(100);
    } else {
        temp+=first_vol;
        i++;
    }
}

void AirQuality::init(void (*IRQ)(void))
{
    unsigned char i = 0;
    aqRefVal = 0;
    PRINTF("Air Quality Sensor Starting Up...(20s)\n\r");
    wait(20); //20s
    init_voltage = _sensor.read() * 1000; // boost the value to be on a 0 -> 1000 scale for compatibility
    PRINTF("The initial voltage is %d%% of source \n\r",init_voltage/10);
    while(init_voltage) {
        if((init_voltage < 798) && (init_voltage > 10)) {
            // the init voltage is ok
            first_vol = (int)_sensor.read() * 1000;//initialize first value
            last_vol = first_vol;
            vol_standard = last_vol;
            PRINTF("Sensor ready.\n\r");
            error = false;;
            break;
        } else if(init_voltage > 798 || init_voltage <= 10) {
            // The sensor is not ready, wait a bit for it to cool off
            i++;
            PRINTF("Sensor not ready (%d), try %d/5, waiting 60 seconds...\n\r",init_voltage,i);
            wait(60);//60s
            init_voltage = (int)_sensor.read() * 1000;
            if(i==5) {
                // After 5 minutes warn user that the sensor may be broken
                i = 0;
                error = true;
                PRINTF("Sensor Error! You may have a bad sensor. :-(\n\r");
            }
        } else
            break;
    }
    // Call AirQualityInterrupt every 2seconds
    IntHandler.attach(IRQ, 2.0);
    PRINTF("Test begin...\n\r");
}

int AirQuality::getAQSensorValue() {
    int val = _sensor.read()*1000;
//    printf("getaq%d ...\r\n", val);
//    wait_ms(100);
    return val;
}

int AirQuality::slope()
{
    while(timer_index) {
        if(first_vol-last_vol > 400 || first_vol > 700) {
            PRINTF("High pollution! Force signal active.\n\r");
            timer_index = 0;
            avgVoltage();
            return 0;
        } else if((first_vol - last_vol > 400 && first_vol < 700) || first_vol - vol_standard > 150) {
            PRINTF("sensor_value:%d",first_vol);
            PRINTF("\t High pollution!\n\r");
            timer_index = 0;
            avgVoltage();
            return 1;

        } else if((first_vol-last_vol > 200 && first_vol < 700) || first_vol - vol_standard > 50) {
            //PRINTF(first_vol-last_vol);
            PRINTF("sensor_value:%d",first_vol);
            PRINTF("\t Low pollution!\n\r");
            timer_index = 0;
            avgVoltage();
            return 2;
        } else {
            avgVoltage();
            PRINTF("sensor_value:%d",first_vol);
            PRINTF("\t Air fresh\n\r");
            timer_index = 0;
            return 3;
        }
    }
    return -1;
}
