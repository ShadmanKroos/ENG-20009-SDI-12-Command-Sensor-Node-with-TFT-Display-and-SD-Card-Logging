# ENG-20009-SDI-12-Command-Sensor-Node-with-TFT-Display-and-SD-Card-Logging

SDI-12 Command Sensor Node with TFT Display & SD Card Logging: This project is an Arduino-based sensor system that combines an SDI-12 style command interface with a button-driven TFT display menu and SD card storage. The firmware reads environmental data from two I2C sensors: Adafruit BME680 (temperature, humidity, pressure, gas resistance) and BH1750 (light level). A ST7735 TFT display shows a menu and visual output (including a temperature chart), while four buttons (interrupt-driven) are used to select which measurement to view.

On the SDI-12 side, the sketch listens on Serial1 at 1200 baud (SERIAL_7E1) and responds to core command patterns implemented in the code:

- Address query `?`: returns the current device address
- Change address `<address>A<digit>...`: updates the address and responds with the new address
- Start measurement `<address>M...`: measures using the BME680 and returns measurement time and number of values
- Send data `<address>D1...`: returns formatted BME680 readings
- Identification `<address>I...`: returns version, unit, and ID fields


For local logging, selected sensor readings are written to an SD card using SdFat (software SPI) into message.txt, and the sketch also includes SD readback routines for printing file contents to the serial monitor.
