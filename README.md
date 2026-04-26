# IR12GJ Methane and Dual-Gas Sensing Platform

This repository contains the software developed for the final-year project on a low-cost methane sensing platform based on the SGX Sensortech IR12GJ NDIR methane sensor. It includes the standalone methane sensor PCB code, the final integrated methane–CO2 data-logging code, and the Python calibration/plotting script.

## Repository structure

- `methane_sensor_pcb_code/`
  - Contains the standalone Arduino code for the methane sensor PCB.
  - Used to drive the IR12GJ lamp and obtain ACT and REF peak-to-peak measurements through the ADS1115 ADC.

- `integrated_dual_gas_code/`
  - Contains the final integrated Arduino code for the combined methane and CO2 sensing platform.
  - Includes methane measurement, CO2 measurement, BMP pressure/temperature measurement, RTC timestamping, SD-card logging, and serial output.

- `calibration_plotting_code/`
  - Contains the Python script used for methane calibration curve fitting and residual plotting.

## Code acknowledgement

The integrated dual-gas code was developed with reference to the public CO2Sensor repository:

`https://github.com/tombishop1/CO2Sensor/blob/main/arduino_sketches/Code_Basic/Code_Basic.ino`

The original CO2 code was created by Sarah Louise Brown at the University of Manchester Geography Department and is associated with the low-cost CO2 sensing platform reported by Brown et al. The integrated code in this repository substantially modifies and extends that basis by adding IR12GJ methane sensing, ADS1115 acquisition, Timer1 lamp chopping, methane calibration equations, BMP sensor integration, and combined methane–CO2 logging.

## Notes

The code was developed for academic project use and may require hardware-specific pin assignments, calibration coefficients, library versions, and file paths to be adjusted before reuse.
