# Methane Sensor PCB Arduino Code

## Introduction

This folder contains the Arduino firmware used for standalone testing and operation of the IR12GJ methane sensor PCB developed for the individual project **Dual-Gas Sensing Platform for Peatland Greenhouse Gas Research**.

The code drives the SGX Sensortech IR12GJ methane sensor lamp at 4 Hz, reads the active and reference detector signals through an ADS1115 ADC, extracts peak-to-peak ACT and REF amplitudes, applies temperature compensation and calibration equations, and outputs methane concentration estimates through the serial monitor.

The purpose of this software is to verify and operate the standalone methane sensing PCB before integration with the wider CO2 sensing platform.

## Folder structure

```text
methane_sensor_pcb_code/
|
├── methane_sensor_pcb_code.ino
└── README.md
```

## Contextual overview

The IR12GJ methane sensor is a dual-channel non-dispersive infrared (NDIR) gas sensor. It has an active detector channel that responds to methane absorption and a reference detector channel used to compensate for source intensity variation, optical drift, and other non-gas-related changes.

The custom methane PCB provides the external support electronics required by the sensor, including lamp-drive circuitry, analogue signal conditioning, and ADC interfacing. The Arduino controls the lamp drive, reads the conditioned detector outputs, calculates the active and reference peak-to-peak amplitudes, and estimates methane concentration.

Simplified workflow:

```text
IR12GJ methane sensor
        ↓
Custom methane sensor PCB
        ↓
Signal-conditioned ACT and REF outputs
        ↓
ADS1115 ADC
        ↓
Arduino peak/trough sampling
        ↓
ACT and REF peak-to-peak amplitudes
        ↓
Temperature compensation and calibration equation
        ↓
Serial output of methane concentration estimate
```

## Main file

### `methane_sensor_pcb_code.ino`

This is the Arduino sketch used for standalone methane sensor PCB testing. It was used before final integration with the CO2 platform to verify the methane PCB, lamp timing, ADC sampling, ACT/REF extraction, temperature reading, and methane concentration calculation.

## Main functions

The code performs the following functions:

- generates a 4 Hz lamp-drive signal for the IR12GJ sensor using Timer1,
- toggles the lamp-drive pin every 125 ms to produce a 4 Hz square wave,
- samples the ACT and REF detector channels using the ADS1115 ADC,
- captures peak and trough values after lamp switching events,
- calculates ACT and REF peak-to-peak amplitudes,
- alternates measurement cycles between the active and reference channels,
- reads the IR12GJ internal temperature sensor channel,
- applies zero, span, alpha, and beta temperature-compensation constants,
- calculates methane concentration using fitted modified Beer-Lambert coefficients,
- outputs `ACT_pp`, `REF_pp`, `Temp_C`, and `CH4_ppm` as CSV-format serial data.

## Required hardware

This code is hardware-dependent and is intended for the standalone methane sensor PCB used in the project. The main hardware components are:

- Arduino Uno,
- custom IR12GJ methane sensor PCB,
- SGX Sensortech IR12GJ methane sensor,
- suitable power supply and wiring.

The code cannot be fully tested without the methane sensor PCB and connected IR12GJ sensor.

## Required Arduino libraries

The following libraries are required:

- `Wire`
- `ADS1X15`
- `TimerOne`

`Wire` is included with the Arduino IDE. `ADS1X15` and `TimerOne` may need to be installed through the Arduino IDE Library Manager or from their official sources.

The code uses the `ADS1X15.h` library interface rather than the `Adafruit_ADS1X15.h` interface. This is important because the function names used in this sketch, such as `ads.toVoltage()` and `ads.readADC()`, depend on the selected ADS1X15 library.

## Installation instructions

1. Install the Arduino IDE.

2. Open the Arduino IDE and install the required libraries:

   ```text
   Sketch -> Include Library -> Manage Libraries
   ```

3. Search for and install:

   ```text
   ADS1X15
   TimerOne
   ```

4. Connect the Arduino Uno to the computer.

5. Select the correct board:

   ```text
   Tools -> Board -> Arduino Uno
   ```

6. Select the correct serial port:

   ```text
   Tools -> Port
   ```

7. Open the file:

   ```text
   methane_sensor_pcb_code.ino
   ```

8. Compile the sketch to check that all libraries are installed correctly.

## How to run the software

1. Connect the standalone methane sensor PCB to the Arduino Uno.

2. Ensure that the IR12GJ sensor, ADS1115 ADC, lamp-drive circuit, ACT channel, REF channel, and temperature sensor channel are connected according to the prototype hardware.

3. Open `methane_sensor_pcb_code.ino` in the Arduino IDE.

4. Upload the sketch to the Arduino Uno.

5. Open the Serial Monitor at:

   ```text
   115200 baud
   ```

6. The serial monitor should first print the CSV header:

   ```text
   ACT_pp,REF_pp,Temp_C,CH4_ppm
   ```

7. The code then prints one row of averaged methane data approximately once per second.

## Serial output format

The serial output has the following columns:

```text
ACT_pp,REF_pp,Temp_C,CH4_ppm
```

where:

- `ACT_pp` is the averaged active-channel peak-to-peak amplitude in volts,
- `REF_pp` is the averaged reference-channel peak-to-peak amplitude in volts,
- `Temp_C` is the averaged IR12GJ sensor temperature in degrees Celsius,
- `CH4_ppm` is the calculated methane concentration in ppm.

The output is formatted as comma-separated values so that it can be copied or logged for later analysis.

## Technical details

### Lamp timing

The IR12GJ lamp is controlled using a Timer1 interrupt. The timer interrupt period is set to 125 ms. Since the interrupt toggles the lamp state each time, the full lamp period is 250 ms, corresponding to a 4 Hz square wave.

```text
Timer interrupt period = 125 ms
Lamp full period = 250 ms
Lamp frequency = 4 Hz
```

### Active and reference channel sampling

The code alternates between ACT and REF measurement cycles. For each channel:

1. after the lamp switches ON, the code samples the selected ADC channel and records the maximum voltage,
2. after the lamp switches OFF, the code samples the selected ADC channel and records the minimum voltage,
3. the peak-to-peak amplitude is calculated as the difference between the maximum and minimum voltages,
4. ACT and REF amplitudes are accumulated over one second before being averaged.

This method is used because the IR12GJ detector outputs are AC signals at the lamp-drive frequency, and methane concentration is calculated from the peak-to-peak active/reference response.

### Temperature measurement

The IR12GJ temperature sensor channel is read through the ADS1115. The voltage is converted to temperature using the relationship for the IR12GJ IC temperature sensor:

```text
T = (V - 0.424) / 0.00625
```

where:

- `T` is temperature in degrees Celsius,
- `V` is the measured temperature-sensor voltage.

### Methane concentration calculation

The code calculates the normalised ratio using:

```text
NR = ACT / (ZERO × REF)
```

Alpha temperature compensation is then applied:

```text
NRcomp = NR × (1 + alpha × (T - Tcal))
```

The span is temperature-compensated using:

```text
SPANcomp = SPAN + beta × ((T - Tcal) / Tcal)
```

The methane concentration is calculated using the rearranged modified Beer-Lambert model:

```text
c = [-ln(1 - ((1 - NRcomp) / SPANcomp)) / a]^(1 / n)
```

where:

- `c` is methane concentration in % v/v,
- `ZERO = 0.99790`,
- `Tcal = 22 °C`,
- `alpha = 0.000277`,
- `SPAN = 0.36154`,
- `a = 0.19393`,
- `n = 0.73034`.

The result is converted from % v/v to ppm using:

```text
CH4_ppm = c × 10000
```

### Beta coefficient selection

The code selects the beta coefficient depending on whether the measured sensor temperature is above or below the calibration temperature:

```text
beta = -0.106 if T >= Tcal
beta = -0.137 if T < Tcal
```

## Design assumptions

The code assumes that:

- the standalone methane sensor PCB is connected to the Arduino Uno,
- the lamp-control circuit is driven from Arduino digital pin 3,
- the ADS1115 uses the channel assignments defined in the code,
- the ACT signal is connected to ADS1115 channel 3,
- the REF signal is connected to ADS1115 channel 2,
- the IR12GJ temperature signal is connected to ADS1115 channel 0,
- the calibration constants are valid for the specific sensor, PCB, and calibration dataset used in the project,
- the serial output is read at 115200 baud.

## Known issues and limitations

- The code is specific to the standalone prototype hardware and may require changes for other wiring or PCB revisions.
- The calibration constants are hard-coded in the sketch.
- The methane concentration estimate is only valid for the calibration setup and range used in the project.
- The code does not log directly to an SD card; it only outputs data through the serial monitor.
- The code does not include extensive fault handling if the ADS1115 is not detected.
- The code does not perform raw-data storage of every ADC sample; it only outputs averaged ACT, REF, temperature, and methane concentration values.
- The code is intended for standalone methane sensor PCB testing and serial-output monitoring.
- Serial data are not saved automatically by the Arduino sketch. To store the output values, use a third-party serial terminal or logging program such as CoolTerm to capture the CSV-formatted serial output.

## Future improvements

Possible improvements include:

- adding explicit startup checks for ADS1115 communication,
- storing calibration constants in EEPROM or a configuration file,
- adding serial commands to change calibration constants without recompiling,
- adding raw ADC sample logging for debugging signal timing,
- separating methane signal acquisition and concentration calculation into reusable functions,

## Academic context

This software was developed as part of the final-year individual project **Dual-Gas Sensing Platform for Peatland Greenhouse Gas Research**. It is included to allow inspection and evaluation of the standalone methane sensor PCB firmware used before final dual-gas system integration.
