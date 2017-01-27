# Binary clock

### Hardware
The hardware platform was designed and made by KBS.

Notable chips used in the project:

* LD1117AL - voltage regulator
* DS1307 - real-time clock module on I2C bus
* AT89S4051 - microcontroller
* DS18B20 - digital thermometer on 1-wire bus
* TSOP1736 - infrared receiver
* SCT2024 - shift register / constant-current LED driver

### Software
My part of work.

* C language
* toolchain - MCU 8051 IDE + SDCC
* infrared protocol - NEC (used in many LG remote controls)
* clock time and temperature are displayed as BCD in binary fashion
* two time display formats - HH:MM and MM:SS
* the clock time is restored after brownout from the RTC module
* nychthemeron adjustment (typical deviation for quartz crystal resonators is about 2-5 seconds per day)
* animation is played for every full hour
* the clock time can be set only after entering the valid passcode
* Snake "game" (you cannot eat food here)

### Other information
Date of project completion - March 2013

In the extras folder there is the source code of the Windows application pretending to be a binary clock.
