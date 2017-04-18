//
// Created by nirao on 18.04.17.
//

#include"mbed.h"
#include"Grove_Air_Quality_Sensor_Library/Air_Quality.h"

int current_quality = -1;
PinName analogPin(PTC1);
DigitalOut extPower(PTC8);
DigitalOut led1(LED1);
AirQuality airqualitysensor;

// Interrupt Handler
void AirQualityInterrupt(void)
{
    AnalogIn sensor(analogPin);
    airqualitysensor.last_vol = airqualitysensor.first_vol;
    airqualitysensor.first_vol = sensor.read()*1000;
    airqualitysensor.timer_index = 1;
}

void ledBlink(void const *args){
    while(1) {
        led1 = !led1;
        Thread::wait(800);
    }
}
osThreadDef(ledBlink, osPriorityNormal, DEFAULT_STACK_SIZE);

// Main loop
int main() {
    osThreadCreate(osThread(ledBlink), NULL);
    extPower.write(1);

        airqualitysensor.init(analogPin, AirQualityInterrupt);
        while (1) {
            current_quality = airqualitysensor.slope();
            if (current_quality >= 0) { // if a valid data returned.
                if (current_quality == 0);
//                    printf("High pollution! Force signal active\n\r");
                else if (current_quality == 1);
//                    printf("High pollution!\n\r");
                else if (current_quality == 2);
//                    printf("Low pollution!\n\r");
                else if (current_quality == 3);
//                    printf("Fresh air\n\r");
            }
        }
}