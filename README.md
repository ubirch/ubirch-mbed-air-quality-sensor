# ubirch-mbed-air-quality-sensor

# Ubirch Air-Quality/ Environmental Sensor

This repo can be used to create Air qulity sensor and also environmental sensor. Set the `AIR_QUALITY_SENSOR` in `mbed_app.json` 
file to 1 for air quality sensor and 0 for environmental sensor

Measure the quality of the air using Seeed Groove-Air quality sensor v1.3 and send data to backend using UBRIDGE sensor

Also the Environmental sensor on the Ubirch#1 board measures the temperature, pressure and humidity asynchronously.

The board first signs these sensor values and sends the signed values to the Ubirch-Backend.
Public-key cryptography is implemented to exchange the messages between the board and the backend securely.

The message is sent to the backend in a predefined interval of time, these intervals can also be configured by sending a configuration message to the device from the server. 
If the sensor temperature is more than the threshold limit then the message is sent more often.

# Getting Started
- Clone the [ubirch-mbed-air-quality-sensor](https://github.com/ubirch/ubirch-mbed-air-quality-sensor)
- Run `mbed deploy` to find and add missing libraries
- Run `mbed update` to update the repo to latest revision/ branch 
- add target `mbed target UBRIDGE`
- add toolchain `mbed toolchain GCC_ARM`
- The board gets the credentials like **Cell APN, username, password, MQTT username, password, host URL and port number** from `config.h` file, which is ignored by git.
 
   Copy/ rename the [config.h.template](https://github.com/ubirch/mbed-os-env-sensor/blob/master/config.h.template) file as `config.h` and add the credentials 

# Building
- to compile the program using mbed build tool run `mbed compile`
- to clean and rebuild the directory again run `mbed compile -c`

# Flashing
You can find the flash script in `bin` directory
- run `./bin/flash.sh` to flash using NXP blhost tool
- alternatively, if you have SEGGER tools installed, run `./bin/flash.sh -j`

# Debugging
- To compile Debug Release
`mbed compile --profile mbed-os/tools/profiles/debug.json`
- use `-c` to recompile everything
- Create a `gdb.init` file and add this
`target extended-remote localhost:2331`
`monitor halt`

- In terminal start the GDB Server
`JLinkGDBServer -if SWD -device MK82FN256xxx15`
- run `cgdb -d arm-none-eabi-gdb -x /home/user/gdb.init ./BUILD/UBIRCH1/GCC_ARM/mbed-os-porting.elf`
