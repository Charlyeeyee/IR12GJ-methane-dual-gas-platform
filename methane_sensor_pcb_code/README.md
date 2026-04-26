# Methane Sensor PCB Code

This folder contains the Arduino code used for the standalone IR12GJ methane sensor PCB.

The code drives the IR12GJ lamp at 4 Hz using a timer interrupt, reads the active and reference detector signals through the ADS1115 ADC, calculates peak-to-peak ACT and REF amplitudes, applies temperature compensation, and outputs methane concentration estimates through the serial monitor.

## Main file

- `methane_sensor_pcb_code.ino`

## Main functions

- Generates a 4 Hz lamp-drive signal for the IR12GJ sensor.
- Samples the ACT and REF detector channels using the ADS1115 ADC.
- Captures peak and trough values after lamp switching events.
- Calculates ACT and REF peak-to-peak amplitudes.
- Reads the IR12GJ internal temperature sensor.
- Applies zero, span, alpha, and beta temperature-compensation constants.
- Calculates methane concentration using the fitted calibration coefficients.
- Outputs `ACT_pp`, `REF_pp`, `Temp_C`, and `CH4_ppm` as CSV-format serial data.

## Required Arduino libraries

- `Wire`
- `ADS1X15`
- `TimerOne`

## Notes

This code was developed for the standalone methane sensor PCB used during the project. The calibration constants, ADS1115 channel assignments, lamp-drive pin, and timing settings are specific to the prototype hardware and may need to be changed before reuse.
