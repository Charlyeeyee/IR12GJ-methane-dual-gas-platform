# Integrated Dual-Gas Code

This folder contains the final Arduino code for the integrated methane and CO2 sensing platform.

## Main file

- `integrated_dual_gas_code.ino`

The code combines the IR12GJ methane sensing module with the existing CO2 sensing platform to enable simultaneous dual-gas measurement. It drives the methane sensor lamp, reads the ACT and REF channels through the ADS1115 ADC, calculates methane signal amplitudes and methane concentration, communicates with the CO2 sensor, records pressure and temperature data, timestamps each reading using the real-time clock, and saves the data to the SD card.

## Main functions

- Drives the IR12GJ methane sensor lamp at 4 Hz using Timer1.
- Reads methane active and reference detector signals using the ADS1115 ADC.
- Calculates ACT and REF peak-to-peak signal amplitudes.
- Applies methane calibration and temperature-compensation equations.
- Calculates methane concentration in ppm.
- Reads CO2 concentration from the K-series CO2 sensor.
- Reads pressure and temperature from the BMP sensor.
- Uses the DS3231 RTC to timestamp measurements.
- Logs data to the SD card as a CSV file.
- Outputs measurement data through the serial monitor.
- Includes SD-card status reporting for each printed row.

## Code origin and acknowledgements

The integrated dual-gas Arduino code was developed for this final-year project by Charles Lin.

The CO2 sensing, RTC timestamping, SD-card logging, and pushbutton logging structure were developed with reference to the public CO2Sensor repository, specifically:

`https://github.com/tombishop1/CO2Sensor/blob/main/arduino_sketches/Code_Basic/Code_Basic.ino`

The original CO2 code was created by Sarah Louise Brown at the University of Manchester Geography Department and is associated with the low-cost CO2 sensing platform reported by Brown et al. The original code also acknowledges code examples from Adafruit, Dejan Nedelkovski, Tom Igoe, and Arduino.cc.

This integrated version has been substantially modified to include the IR12GJ methane sensing module, ADS1115 ACT/REF acquisition, Timer1 lamp chopping, methane calibration equations, BMP sensor integration, 1-second methane averaging, and combined methane–CO2 SD-card logging.

The original standalone CO2 code is not reproduced separately in this repository; only the final integrated version used for this project is included.

## Required Arduino libraries

- `Wire`
- `SPI`
- `SD`
- `Adafruit_ADS1X15`
- `TimerOne`
- `KSeries`
- `RTClib`
- `Adafruit_BMP3XX`
- `math.h`

## Notes

This code was developed for the final integrated prototype used in the project. Pin assignments, calibration coefficients, sensor libraries, SD-card settings, and RTC settings are specific to the prototype hardware and may need to be changed before reuse with different hardware.
