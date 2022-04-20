# ADS1119

This is a slightly modified version of ADS1119 Arduino library from ELOWRO. Rewritten in a way that now it is not using the Arduino-related libraries and targetting to be compatible with the ESP32 platform, it also utilizes a thread-safe I2C Manager by ropg (https://github.com/ropg/i2c_manager).

To be able to use it, you first need to have the i2c_manager repository in the components directory. I2C ports then have to be enabled and setup individually via menuconfig. You can find more information here: https://github.com/ropg/i2c_manager#simplest-usage
