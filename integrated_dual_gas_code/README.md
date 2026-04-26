# Integrated Dual-Gas Code

This folder contains the final Arduino code for the integrated methane and CO₂ sensing platform.

The code combines the IR12GJ methane sensing module with the existing CO₂ sensing platform to enable simultaneous dual-gas measurement. It drives the methane sensor lamp, reads the ACT and REF channels through the ADS1115 ADC, calculates methane signal amplitudes, communicates with the CO₂ sensor, records environmental measurements, timestamps each reading using the real-time clock, and saves the data to the SD card.

## Main functions

- Drives the IR12GJ methane sensor lamp at 4 Hz.
- Reads methane active and reference detector signals using the ADS1115 ADC.
- Calculates peak-to-peak ACT and REF signal amplitudes.
- Reads CO₂ concentration from the CO₂ sensor.
- Records pressure and temperature data using the BMP sensor.
- Uses the RTC to timestamp measurements.
- Logs data to the SD card.
- Outputs measurement data through the serial monitor.

## Notes

This code was developed for the final integrated prototype used in the project. Pin assignments, calibration coefficients, sensor libraries, and SD-card settings may need to be changed before reuse with different hardware.
