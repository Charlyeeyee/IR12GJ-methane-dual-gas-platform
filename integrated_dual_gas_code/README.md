# Integrated Dual-Gas Arduino Code

## Introduction

This folder contains the final Arduino firmware for the integrated methane and CO2 sensing platform developed for the individual project **Dual-Gas Sensing Platform for Peatland Greenhouse Gas Research**.

The code integrates the SGX Sensortech IR12GJ methane sensing PCB with an existing low-cost CO2 sensing platform. It performs methane signal acquisition using an ADS1115 ADC and Timer1-based lamp chopping, while also reading CO2 concentration, pressure, temperature, real-time-clock timestamps, pushbutton events, and SD-card logging status.

The purpose of this software is to operate the final integrated prototype used in the project and to produce timestamped dual-gas measurement data for methane and CO2.

## Folder structure

```text
integrated_dual_gas_code/
|
├── integrated_dual_gas_code.ino
└── README.md
```

## Contextual overview

The integrated system combines two sensing subsystems:

1. an IR12GJ methane sensing module with a custom signal-conditioning PCB, and
2. a low-cost CO2 sensing platform using a K-series CO2 sensor.

The methane PCB drives the IR12GJ infrared lamp at 4 Hz and conditions the active and reference detector signals. The Arduino reads these conditioned signals through an ADS1115 ADC, extracts peak-to-peak amplitudes, applies calibration and temperature compensation, and calculates methane concentration.

The integrated platform also reads CO2 concentration, BMP pressure and temperature data, DS3231 real-time-clock timestamps, and pushbutton event counts. The combined data are written to an SD card and printed to the serial monitor once per second.

Simplified system workflow:

```text
IR12GJ methane sensor
        ↓
Methane signal-conditioning PCB
        ↓
ADS1115 ADC
        ↓
Arduino methane peak-to-peak extraction
        ↓
Methane calibration and temperature compensation
        ↓
Combined with CO2, BMP, RTC, and button data
        ↓
Serial output and SD-card CSV logging
```

## Main file

### `integrated_dual_gas_code.ino`

This is the final Arduino sketch for the integrated dual-gas prototype. It combines methane acquisition, CO2 measurement, pressure/temperature measurement, timestamping, pushbutton event counting, and SD-card logging.

## Main functions

The code performs the following functions:

- drives the IR12GJ methane sensor lamp at 4 Hz using Timer1,
- reads methane active and reference detector signals using the ADS1115 ADC,
- extracts ACT and REF peak-to-peak signal amplitudes,
- averages methane measurements over one-second intervals,
- applies methane calibration and temperature-compensation equations,
- calculates methane concentration in ppm,
- reads CO2 concentration from the K-series CO2 sensor,
- reads pressure and temperature from the BMP sensor,
- uses the DS3231 real-time clock to timestamp measurements,
- counts pushbutton events during data collection,
- logs combined data to the SD card as a CSV file,
- outputs measurement data through the serial monitor,
- reports SD-card write status for each printed row.

## Required hardware

This code is hardware-dependent and is intended for the final integrated prototype. The main hardware components are:

- Arduino Uno,
- custom IR12GJ methane sensor PCB,
- custom integrated CO2 sensor PCB,
- SGX Sensortech IR12GJ methane sensor,
- K-series CO2 sensor,
- BMP390 breakout board,
- SD-card,
- appropriate power supply and wiring.

The code cannot be fully tested without the connected sensor hardware.

## Required Arduino libraries

The following libraries are required:

- `Wire`
- `SPI`
- `SD`
- `Adafruit_ADS1X15`
- `TimerOne`
- `KSeries`
- `RTClib`
- `Adafruit_BMP3XX`
- `math.h`

`Wire`, `SPI`, `SD`, and `math.h` are standard Arduino libraries or standard C/C++ libraries. The remaining libraries may need to be installed through the Arduino IDE Library Manager or from their official sources.

## Installation instructions

1. Install the Arduino IDE.

2. Open the Arduino IDE and install the required libraries:

   ```text
   Sketch -> Include Library -> Manage Libraries
   ```

3. Search for and install the required third-party libraries, including:

   ```text
   Adafruit ADS1X15
   TimerOne
   RTClib
   Adafruit BMP3XX
   ```

4. Install the `KSeries` library used for the K-series CO2 sensor. If it is not available through the Arduino Library Manager, install it from the source used by the existing CO2 sensing platform.

5. Connect the Arduino Uno and select:

   ```text
   Tools -> Board -> Arduino Uno
   ```

6. Select the correct serial port:

   ```text
   Tools -> Port
   ```

7. Open the file:

   ```text
   integrated_dual_gas_code.ino
   ```

8. Compile the sketch to check that all libraries are installed correctly.

## How to run the software

1. Connect the integrated hardware platform to the Arduino Uno.

2. Insert an SD card into the SD-card module.

3. Connect the Arduino Uno to the computer by USB.

4. Open `integrated_dual_gas_code.ino` in the Arduino IDE.

5. Upload the sketch to the Arduino Uno.

6. Open the Serial Monitor at:

   ```text
   115200 baud
   ```

7. The serial monitor should print one row of data approximately once per second.

8. If the SD card is working correctly, each serial row should begin with:

   ```text
   SD=OK
   ```

   If SD-card writing fails, the row begins with:

   ```text
   SD=FAIL
   ```

9. Logged data are written to the SD card file:

   ```text
   datalog.csv
   ```

## Serial and SD-card output

The code logs and prints the following values:

```text
ms, ACT_uV, REF_uV, CH4ppm, SensT_c, PCBT_c, CO2ppm, PressPa, BMPT_c, Btn, date, time
```

where:

- `ms` is elapsed time in milliseconds,
- `ACT_uV` is the active methane channel peak-to-peak amplitude in microvolts,
- `REF_uV` is the reference methane channel peak-to-peak amplitude in microvolts,
- `CH4ppm` is the calculated methane concentration in ppm,
- `SensT_c` is the IR12GJ sensor temperature multiplied by 100,
- `PCBT_c` is the PCB temperature multiplied by 100,
- `CO2ppm` is the CO2 concentration in ppm,
- `PressPa` is pressure in pascals,
- `BMPT_c` is BMP temperature multiplied by 100,
- `Btn` is the pushbutton event count,
- `date` is the RTC date,
- `time` is the RTC time.

## Technical details

### Methane lamp timing

The IR12GJ methane sensor lamp is controlled using Timer1. The interrupt toggles the lamp-control output every 125 ms, producing a 250 ms full period. This corresponds to a 4 Hz lamp-drive frequency.

### Methane signal acquisition

The methane sensor produces active and reference detector signals. These are measured using the ADS1115 ADC. The code alternates between active and reference measurement cycles because changing ADS1115 input channels introduces sampling overhead.

For each channel:

1. the maximum value is sampled after a lamp rising edge,
2. the minimum value is sampled after the following lamp falling edge,
3. the peak-to-peak amplitude is calculated from the difference,
4. valid amplitudes are accumulated for one-second averaging.

### Methane concentration calculation

The code calculates methane concentration using the active and reference peak-to-peak amplitudes. The normalised ratio is calculated as:

```text
NR = ACT / (ZERO × REF)
```

Alpha temperature compensation is then applied:

```text
NRcomp = NR × (1 + alpha × (T - Tcal))
```

The span is also temperature-compensated using the beta coefficient:

```text
SPANcomp = SPAN + beta × ((T - Tcal) / Tcal)
```

The methane concentration is then calculated using the rearranged modified Beer-Lambert model:

```text
c = [-ln(1 - ((1 - NRcomp) / SPANcomp)) / a]^(1 / n)
```

where:

- `ZERO = 0.99790`,
- `Tcal = 22.0 °C`,
- `alpha = 0.000277`,
- `SPAN = 0.36154`,
- `a = 0.19393`,
- `n = 0.73034`.

The result is converted from % v/v to ppm by multiplying by 10,000.

### Temperature compensation

The code uses different beta values depending on whether the sensor temperature is above or below the calibration temperature:

```text
beta = -0.106 if T >= Tcal
beta = -0.137 if T < Tcal
```

### One-second maintenance routine

Once per second, the code performs slower operations including:

- reading the RTC,
- reading CO2 concentration,
- reading BMP pressure and temperature,
- writing to the SD card,
- printing one serial output row.

Any methane lamp-edge flags generated during these slower operations are discarded afterwards to prevent invalid peak-pairing between old and new measurement cycles.

## Design assumptions

The code assumes that:

- the Arduino Uno is connected to the same prototype hardware used in the project,
- the IR12GJ sensor is conncted to the methane sensor PCB,
- the calibration constants correspond to the specific methane sensor PCB and calibration dataset used in the project,
- the SD card is present and writable before data collection begins.

## Known issues and limitations

- The code is specific to the final prototype hardware and may require pin or library changes for other hardware.
- The methane calibration coefficients are hard-coded.
- The integrated system cannot be fully validated without the custom methane PCB and connected sensors.
- Slow operations such as SD-card writing and sensor communication can interfere with methane edge timing; the code manages this by discarding edge flags after the one-second maintenance routine.
- The SD-card file is always named `datalog.csv`.
- The code performs limited runtime fault recovery if a sensor or SD card fails after startup.
- The methane concentration calculation is only valid for the calibration range and setup used in the project.

## Future improvements

Possible improvements include:

- moving calibration coefficients into a configuration file or EEPROM storage,
- adding stronger fault handling for SD-card and sensor failures,
- adding automatic file naming to avoid overwriting or appending to old logs,
- adding more detailed startup diagnostics,
- adapting the firmware to a microcontroller with more memory and hardware serial ports.

## Code origin and acknowledgements

The integrated dual-gas Arduino code was developed for this final-year project by Charles Lin.

The CO2 sensing, RTC timestamping, SD-card logging, and pushbutton logging structure were developed with reference to the public CO2Sensor repository, specifically:

```text
https://github.com/tombishop1/CO2Sensor/blob/main/arduino_sketches/Code_Basic/Code_Basic.ino
```

The original CO2 code was created by Sarah Louise Brown at the University of Manchester Geography Department and is associated with the low-cost CO2 sensing platform reported by Brown et al.

The original code also acknowledges code examples from Adafruit, Dejan Nedelkovski, Tom Igoe, and Arduino.cc for sensor, SD-card, RTC, and pushbutton functionality.

This integrated version has been substantially modified by Charles Lin to add IR12GJ methane sensing, ADS1115 ACT/REF signal acquisition, Timer1 lamp chopping, methane calibration equations, BMP sensor integration, one-second methane averaging, and combined methane-CO2 SD-card logging.

The original standalone CO2 code is not reproduced separately in this repository. Only the final integrated version used for this project is included.

## Academic context

This software was developed as part of the final-year individual project **Dual-Gas Sensing Platform for Peatland Greenhouse Gas Research**. It is included to allow inspection and evaluation of the final integrated Arduino firmware used in the project.
