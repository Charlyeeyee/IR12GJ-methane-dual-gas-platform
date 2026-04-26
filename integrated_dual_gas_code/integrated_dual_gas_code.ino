/*
 * Integrated Methane and CO2 Sensing Platform
 * Final-year individual project
 *
 * Author: Charles Lin
 *
 * Purpose:
 * This program integrates the SGX Sensortech IR12GJ methane sensor PCB
 * with an existing low-cost CO2 sensing platform. It performs methane
 * signal acquisition using an ADS1115 ADC and Timer1-based lamp chopping,
 * while also reading CO2 concentration, BMP pressure/temperature data,
 * RTC timestamps, button state, and logging all data to an SD card.
 *
 * Code acknowledgement:
 * The CO2 sensing, RTC timestamping, SD-card logging, and pushbutton
 * logging structure were developed with reference to the public CO2Sensor
 * repository, specifically:
 *
 * https://github.com/tombishop1/CO2Sensor/blob/main/arduino_sketches/Code_Basic/Code_Basic.ino
 *
 * The original CO2 code was created by Sarah Louise Brown at the University
 * of Manchester Geography Department as part of doctoral research, and is
 * associated with:
 *
 * Brown, S. L.; Goulsbra, C. G.; Evans, M. G.; Heath, T.
 * Low Cost CO2 Sensing: A Simple Microcontroller Approach in the Context
 * of Peatland Fluvial Carbon.
 *
 * The original CO2 code also acknowledged code examples from Adafruit,
 * Dejan Nedelkovski, Tom Igoe, and Arduino.cc for sensor, SD-card, RTC,
 * and pushbutton functionality.
 *
 * This integrated version has been substantially modified by Charles Lin
 * to add IR12GJ methane sensing, ADS1115 ACT/REF signal acquisition,
 * Timer1 lamp chopping, methane calibration equations, BMP sensor
 * integration, 1-second methane averaging, and combined methane-CO2
 * SD-card logging.
 *
 * Third-party libraries used:
 * Wire, SPI, SD, Adafruit_ADS1X15, TimerOne, KSeries, RTClib,
 * Adafruit_BMP3XX, and math.h.
 */


// =============================================================================
// LIBRARIES
// =============================================================================

#include <Wire.h>              // I2C communication library
#include <SPI.h>               // SPI communication library, used by the SD card module
#include <SD.h>                // SD-card file handling library
#include <Adafruit_ADS1X15.h>  // Adafruit ADS1115 ADC library
#include <TimerOne.h>          // Timer1 interrupt library for methane lamp chopping
#include <KSeries.h>           // K-series CO2 sensor library
#include <RTClib.h>            // DS3231 real-time clock library
#include <Adafruit_BMP3XX.h>   // BMP3XX pressure/temperature sensor library
#include <math.h>              // Mathematical functions, including log() and pow()


// =============================================================================
// METHANE SENSOR HARDWARE DEFINITIONS
// =============================================================================

// ADS1115 object used for methane detector and temperature measurements.
Adafruit_ADS1115 ads;

// Arduino output pin used to drive the IR12GJ lamp-control circuit.
const int lampPin = 3;

// ADS1115 channel assignments.
const int sensor_temp = 0;   // IR12GJ internal sensor temperature channel
const int PCB_temp    = 1;   // PCB temperature channel
const int ACT         = 3;   // Active methane detector channel
const int REF         = 2;   // Reference detector channel

// Lamp timing.
// A 125 ms half-period gives a 250 ms full period, corresponding to 4 Hz.
const unsigned long HALF_PERIOD_US = 125000;


// =============================================================================
// TIMER INTERRUPT FLAGS
// =============================================================================

// These variables are modified inside the interrupt routine, so they are volatile.
volatile bool lampState = false;         // Current lamp state: false = OFF, true = ON
volatile bool RisingEdgeFlag  = false;   // Set when the lamp switches ON
volatile bool FallingEdgeFlag = false;   // Set when the lamp switches OFF


// =============================================================================
// TIMER INTERRUPT SERVICE ROUTINE
// =============================================================================

void onTickISR() {
  // Toggle lamp state every 125 ms.
  lampState = !lampState;

  // Apply the new lamp state to the lamp-drive pin.
  digitalWrite(lampPin, lampState);

  // Set the appropriate edge flag for the main loop to process.
  if (lampState) {
    RisingEdgeFlag  = true;    // Lamp has switched ON
    FallingEdgeFlag = false;
  } else {
    RisingEdgeFlag  = false;
    FallingEdgeFlag = true;    // Lamp has switched OFF
  }
}


// =============================================================================
// METHANE CALIBRATION CONSTANTS
// =============================================================================

// Zero value determined from zero-gas calibration.
float zero  = 0.99790;

// Calibration temperature in degrees Celsius.
float Tcal  = 22.0f;

// Alpha coefficient used for zero/baseline temperature compensation.
float alpha = 0.000277f;

// Beta coefficient used for span temperature compensation.
// This is updated in calculateCH4ppm() depending on whether temperature is
// above or below Tcal.
float beta  = -0.106f;

// Fixed SPAN value determined experimentally from methane exposure.
float span  = 0.36154f;

// Fitted linearisation coefficients used in the modified Beer-Lambert model.
float a     = 0.19393f;
float n     = 0.73034f;


// =============================================================================
// METHANE MEASUREMENT CONTROL VARIABLES
// =============================================================================

// Alternates measurement cycles between active and reference detector channels.
// true = active detector cycle; false = reference detector cycle.
bool measureAct = true;

// Per-cycle peak and trough values.
float act_max = 0.0f;
float act_min = 0.0f;
float ref_max = 0.0f;
float ref_min = 0.0f;

// These flags prevent an old maximum value from being paired with a new minimum
// value after slow operations such as SD-card writing.
bool haveActMax = false;
bool haveRefMax = false;


// =============================================================================
// ONE-SECOND METHANE AVERAGING VARIABLES
// =============================================================================

// Accumulators for one-second averaging.
float sumACT   = 0.0f;
float sumREF   = 0.0f;
float sumSensT = 0.0f;
float sumPCBT  = 0.0f;

// Sample counters for averaging.
uint16_t cntACT    = 0;
uint16_t cntREF    = 0;
uint16_t cntCycles = 0;

// Timestamp used to trigger the 1 Hz maintenance/logging routine.
unsigned long last1sMs = 0;

// Latest one-second averaged values.
float avgACT   = 0.0f;
float avgREF   = 0.0f;
float avgSensT = 0.0f;
float avgPCBT  = 0.0f;
float ch4ppm   = 0.0f;


// =============================================================================
// AUXILIARY HARDWARE DEFINITIONS
// =============================================================================

// K-series CO2 sensor using software serial pins 6 and 7.
kSeries K_30(6, 7);

// DS3231 real-time clock object.
RTC_DS3231 rtc;

// BMP3XX pressure/temperature sensor object.
Adafruit_BMP3XX bmp;

// SD-card chip-select pin.
const int pinCS = 10;

// Output pin used as a simple CO2 threshold indicator.
const int twowire = 5;

// Pushbutton input pin.
const int buttonPin = 8;

// Pushbutton counter variables.
int buttonCount = 0;
int lastButtonState = HIGH;


// =============================================================================
// ATOMIC FLAG HELPER FUNCTIONS
// =============================================================================

bool takeRisingFlag() {
  // Read and clear the rising-edge flag atomically to avoid interrupt conflicts.
  noInterrupts();
  bool f = RisingEdgeFlag;
  RisingEdgeFlag = false;
  interrupts();

  return f;
}


bool takeFallingFlag() {
  // Read and clear the falling-edge flag atomically to avoid interrupt conflicts.
  noInterrupts();
  bool f = FallingEdgeFlag;
  FallingEdgeFlag = false;
  interrupts();

  return f;
}


void clearEdgeFlagsAndPairState() {
  // Clear any edge flags generated during slow operations such as CO2 reading,
  // BMP reading, or SD-card writing.
  noInterrupts();
  RisingEdgeFlag = false;
  FallingEdgeFlag = false;
  interrupts();

  // Prevent a maximum sampled before maintenance from being paired with a
  // minimum sampled after maintenance.
  haveActMax = false;
  haveRefMax = false;
}


// =============================================================================
// METHANE PEAK-PICKING FUNCTIONS
// =============================================================================

float intenseRisingSampling(int ch) {
  /*
   * Samples the selected ADS1115 channel after a lamp rising edge and returns
   * the maximum voltage observed over the sampling window.
   *
   * readADC_SingleEnded() is blocking in the Adafruit ADS1115 library, so no
   * additional delay is inserted between samples.
   */

  // Discard first reading after channel selection to reduce channel-switching error.
  (void)ads.readADC_SingleEnded(ch);

  // Initialise maximum voltage to a value below the expected signal range.
  float mx = -10.0f;

  // Acquire 20 samples and retain the maximum.
  for (int i = 0; i < 20; i++) {
    int16_t raw = ads.readADC_SingleEnded(ch);  // Read raw ADC counts
    float v = ads.computeVolts(raw);            // Convert ADC counts to voltage

    if (v > mx) mx = v;
  }

  return mx;
}


float intenseFallingSampling(int ch) {
  /*
   * Samples the selected ADS1115 channel after a lamp falling edge and returns
   * the minimum voltage observed over the sampling window.
   */

  // Discard first reading after channel selection to reduce channel-switching error.
  (void)ads.readADC_SingleEnded(ch);

  // Initialise minimum voltage to a value above the expected signal range.
  float mn = 10.0f;

  // Acquire 20 samples and retain the minimum.
  for (int i = 0; i < 20; i++) {
    int16_t raw = ads.readADC_SingleEnded(ch);  // Read raw ADC counts
    float v = ads.computeVolts(raw);            // Convert ADC counts to voltage

    if (v < mn) mn = v;
  }

  return mn;
}


// =============================================================================
// METHANE CONCENTRATION CALCULATION
// =============================================================================

float calculateCH4ppm(float avgACT_v, float avgREF_v, float tempC) {
  /*
   * Calculates methane concentration in ppm from averaged active/reference
   * peak-to-peak amplitudes and sensor temperature.
   *
   * The calculation applies:
   * - active/reference normalisation;
   * - alpha temperature compensation;
   * - beta span temperature compensation;
   * - modified Beer-Lambert linearisation.
   */

  // Avoid division by zero if the reference signal is invalid.
  if (avgREF_v <= 0.0f) return 0.0f;

  // Select beta coefficient depending on whether temperature is above or below Tcal.
  beta = (tempC >= Tcal) ? -0.106f : -0.137f;

  // Calculate normalised ratio.
  float NR = avgACT_v / (zero * avgREF_v);

  // Apply alpha temperature compensation to the normalised ratio.
  float NRcomp = NR * (1.0f + alpha * (tempC - Tcal));

  // Apply beta temperature compensation to the span.
  float Spancomp = span + beta * ((tempC - Tcal) / Tcal);

  // Reject invalid compensated span values.
  if (Spancomp <= 0.0f) return 0.0f;

  // Calculate the absorbance-related term.
  float absorbancePart = (1.0f - NRcomp) / Spancomp;

  // Prevent invalid input to log(1 - absorbancePart).
  if (absorbancePart >= 1.0f) absorbancePart = 0.9999f;

  // Rearranged modified Beer-Lambert expression.
  float arg = -log(1.0f - absorbancePart);

  // Clamp negative values to zero to avoid invalid concentration outputs.
  if (arg < 0.0f) arg = 0.0f;

  // Divide by fitted coefficient a.
  float base = arg / a;

  // Prevent invalid input to pow().
  if (base < 0.0f) base = 0.0f;

  // Calculate methane concentration in % v/v.
  float c_percent = pow(base, 1.0f / n);

  // Convert % v/v to ppm.
  float c_ppm = c_percent * 10000.0f;

  return c_ppm;
}


// =============================================================================
// PUSHBUTTON UPDATE
// =============================================================================

void updateButton() {
  /*
   * Detects falling-edge button presses using the internal pull-up resistor.
   * The button count can be used to mark events during data collection.
   */

  int s = digitalRead(buttonPin);

  // With INPUT_PULLUP, a button press pulls the input LOW.
  if (lastButtonState == HIGH && s == LOW) {
    buttonCount++;
    delayMicroseconds(2000);  // Short debounce delay
  }

  lastButtonState = s;
}


// =============================================================================
// ONE-HERTZ MAINTENANCE, LOGGING, AND SERIAL OUTPUT
// =============================================================================

void maintenance_1Hz() {
  /*
   * Performs slower 1 Hz tasks:
   * - RTC timestamp reading;
   * - CO2 measurement;
   * - BMP pressure/temperature measurement;
   * - SD-card logging;
   * - serial output.
   *
   * The Timer1 methane lamp interrupt is not paused during this routine.
   * Any edge flags generated during the slow operations are discarded at the end,
   * because they are no longer phase-safe for peak picking.
   */

  // Read current date and time from the RTC.
  DateTime dt = rtc.now();

  // Read current elapsed time in milliseconds.
  unsigned long ms = millis();

  // Read CO2 concentration in ppm from the K-series sensor.
  int co2ppm = K_30.getCO2('p');

  // Read pressure and temperature from the BMP sensor.
  bool bmpOK = bmp.performReading();

  // Store invalid values if BMP reading fails.
  int32_t pressPa = bmpOK ? (int32_t)bmp.pressure : -1;
  int16_t bmpTC   = bmpOK ? (int16_t)(bmp.temperature * 100.0f) : -32768;

  // Set output pin HIGH if CO2 exceeds threshold.
  digitalWrite(twowire, (co2ppm >= 700) ? HIGH : LOW);

  // Convert floating-point measurements to integer units before logging.
  // This reduces SRAM use and produces compact CSV output.
  int32_t act_uV = (int32_t)(avgACT * 1000000.0f);
  int32_t ref_uV = (int32_t)(avgREF * 1000000.0f);
  int16_t sensTC = (int16_t)(avgSensT * 100.0f);
  int16_t pcbTC  = (int16_t)(avgPCBT * 100.0f);
  int32_t ch4ppm_int = (int32_t)ch4ppm;


  // ---------------------------------------------------------------------------
  // SD-CARD LOGGING
  // ---------------------------------------------------------------------------

  bool sdOK = false;

  // Open the CSV log file. If it does not exist, it will be created.
  File f = SD.open("datalog.csv", FILE_WRITE);

  if (f) {
    // If the file is new and empty, write the column header.
    if (f.size() == 0) {
      f.println(F("ms,ACT_uV,REF_uV,CH4ppm,SensT_c,PCBT_c,CO2ppm,PressPa,BMPT_c,Btn,date,time"));
    }

    // Write elapsed time and methane measurements.
    f.print(ms); f.print(',');
    f.print(act_uV); f.print(',');
    f.print(ref_uV); f.print(',');
    f.print(ch4ppm_int); f.print(',');
    f.print(sensTC); f.print(',');
    f.print(pcbTC);  f.print(',');

    // Write CO2 and BMP measurements.
    f.print(co2ppm); f.print(',');
    f.print(pressPa); f.print(',');
    f.print(bmpTC);  f.print(',');

    // Write event/button count.
    f.print(buttonCount); f.print(',');

    // Write date in YYYY-MM-DD format.
    f.print(dt.year()); f.print('-');
    if (dt.month() < 10) f.print('0');
    f.print(dt.month()); f.print('-');
    if (dt.day() < 10) f.print('0');
    f.print(dt.day()); f.print(',');

    // Write time in HH:MM:SS format.
    if (dt.hour() < 10) f.print('0');
    f.print(dt.hour()); f.print(':');
    if (dt.minute() < 10) f.print('0');
    f.print(dt.minute()); f.print(':');
    if (dt.second() < 10) f.print('0');
    f.println(dt.second());

    // Close the file to ensure data are written to the SD card.
    f.close();

    // Mark SD-card write as successful.
    sdOK = true;
  }


  // ---------------------------------------------------------------------------
  // SERIAL OUTPUT
  // ---------------------------------------------------------------------------

  // Print SD-card status first so each serial row indicates whether logging worked.
  Serial.print(F("SD="));
  Serial.print(sdOK ? F("OK ") : F("FAIL "));

  // Print the same data to the serial monitor for live checking.
  Serial.print(ms); Serial.print(',');
  Serial.print(act_uV); Serial.print(',');
  Serial.print(ref_uV); Serial.print(',');
  Serial.print(ch4ppm_int); Serial.print(',');
  Serial.print(sensTC); Serial.print(',');
  Serial.print(pcbTC); Serial.print(',');
  Serial.print(co2ppm); Serial.print(',');
  Serial.print(pressPa); Serial.print(',');
  Serial.print(bmpTC); Serial.print(',');
  Serial.print(buttonCount); Serial.print(',');

  // Print date.
  Serial.print(dt.year()); Serial.print('-');
  if (dt.month() < 10) Serial.print('0');
  Serial.print(dt.month()); Serial.print('-');
  if (dt.day() < 10) Serial.print('0');
  Serial.print(dt.day()); Serial.print(' ');

  // Print time.
  if (dt.hour() < 10) Serial.print('0');
  Serial.print(dt.hour()); Serial.print(':');
  if (dt.minute() < 10) Serial.print('0');
  Serial.print(dt.minute()); Serial.print(':');
  if (dt.second() < 10) Serial.print('0');
  Serial.println(dt.second());

  // Discard any methane edge flags generated during slow maintenance tasks.
  clearEdgeFlagsAndPairState();
}


// =============================================================================
// SETUP
// =============================================================================

void setup() {
  // Start serial communication for debugging and live data output.
  Serial.begin(115200);

  // Configure methane lamp-drive pin.
  pinMode(lampPin, OUTPUT);
  digitalWrite(lampPin, LOW);

  // Configure SD-card chip-select pin.
  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);

  // Configure CO2 threshold output pin.
  pinMode(twowire, OUTPUT);
  digitalWrite(twowire, LOW);

  // Configure pushbutton using the internal pull-up resistor.
  pinMode(buttonPin, INPUT_PULLUP);

  // Start I2C bus and use 400 kHz fast mode.
  Wire.begin();
  Wire.setClock(400000);

  // Initialise ADS1115 ADC.
  if (!ads.begin()) {
    Serial.println(F("ADS1115 FAIL"));
    while (1);
  }

  // Configure ADS1115 input range and sampling rate.
  ads.setGain(GAIN_ONE);                    // ±4.096 V full-scale range
  ads.setDataRate(RATE_ADS1115_860SPS);     // 860 samples/s maximum rate

  // Initialise RTC.
  if (!rtc.begin()) {
    Serial.println(F("RTC FAIL"));
  } else {
    // If RTC has lost power, set time from compilation time.
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Initialise BMP pressure/temperature sensor.
  if (!bmp.begin_I2C()) {
    Serial.println(F("BMP FAIL"));
  }

  // Initialise SD card.
  if (!SD.begin(pinCS)) {
    Serial.println(F("SD INIT FAIL"));
  }

  // Start Timer1 methane lamp chopping.
  Timer1.initialize(HALF_PERIOD_US);
  Timer1.attachInterrupt(onTickISR);

  // Initialise 1-second timing variable.
  last1sMs = millis();

  // Print serial column header.
  Serial.println(F("SD_status ms,ACT_uV,REF_uV,CH4ppm,SensT_c,PCBT_c,CO2ppm,PressPa,BMPT_c,Btn,date time"));
}


// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
  // Update pushbutton state as frequently as possible.
  updateButton();


  // ---------------------------------------------------------------------------
  // RISING EDGE: SAMPLE MAXIMUM
  // ---------------------------------------------------------------------------

  if (takeRisingFlag()) {
    if (measureAct) {
      // Sample active detector maximum after lamp turns ON.
      act_max = intenseRisingSampling(ACT);
      haveActMax = true;
    } else {
      // Sample reference detector maximum after lamp turns ON.
      ref_max = intenseRisingSampling(REF);
      haveRefMax = true;
    }
  }


  // ---------------------------------------------------------------------------
  // FALLING EDGE: SAMPLE MINIMUM AND CALCULATE PEAK-TO-PEAK AMPLITUDE
  // ---------------------------------------------------------------------------

  if (takeFallingFlag()) {
    if (measureAct) {
      // Only calculate active amplitude if a valid active maximum exists.
      if (haveActMax) {
        act_min = intenseFallingSampling(ACT);
        float ACTamp = act_max - act_min;

        // Accumulate only physically valid positive amplitudes.
        if (ACTamp > 0.0f) {
          sumACT += ACTamp;
          cntACT++;
        }

        // Clear pairing flag after use.
        haveActMax = false;
      }
    } else {
      // Only calculate reference amplitude if a valid reference maximum exists.
      if (haveRefMax) {
        ref_min = intenseFallingSampling(REF);
        float REFamp = ref_max - ref_min;

        // Accumulate only physically valid positive amplitudes.
        if (REFamp > 0.0f) {
          sumREF += REFamp;
          cntREF++;
        }

        // Clear pairing flag after use.
        haveRefMax = false;
      }
    }

    // Read temperature channels after each completed or attempted measurement cycle.
    int16_t rawS = ads.readADC_SingleEnded(sensor_temp);
    int16_t rawP = ads.readADC_SingleEnded(PCB_temp);

    // Convert raw ADC counts to voltages.
    float vS = ads.computeVolts(rawS);
    float vP = ads.computeVolts(rawP);

    // Convert temperature-sensor voltage to °C.
    // This follows the IRxxGx IC sensor relationship:
    // V = 0.424 V + 0.00625 V/°C × T.
    float tS = (vS - 0.424f) / 0.00625f;
    float tP = (vP - 0.424f) / 0.00625f;

    // Accumulate temperature readings for 1-second averaging.
    sumSensT += tS;
    sumPCBT  += tP;
    cntCycles++;

    // Alternate between active and reference detector cycles.
    measureAct = !measureAct;
  }


  // ---------------------------------------------------------------------------
  // ONE-SECOND AVERAGING AND LOGGING
  // ---------------------------------------------------------------------------

  unsigned long now = millis();

  if (now - last1sMs >= 1000UL) {
    // Advance by exactly 1000 ms to reduce long-term timing drift.
    last1sMs += 1000UL;

    if (cntCycles > 0) {
      // Update averaged values only when valid samples exist.
      avgACT   = (cntACT > 0) ? (sumACT / cntACT) : avgACT;
      avgREF   = (cntREF > 0) ? (sumREF / cntREF) : avgREF;
      avgSensT = sumSensT / cntCycles;
      avgPCBT  = sumPCBT  / cntCycles;

      // Calculate methane concentration from averaged ACT/REF and temperature.
      ch4ppm = calculateCH4ppm(avgACT, avgREF, avgSensT);
    }

    // Reset one-second methane accumulators.
    sumACT = 0.0f;
    sumREF = 0.0f;
    sumSensT = 0.0f;
    sumPCBT = 0.0f;

    cntACT = 0;
    cntREF = 0;
    cntCycles = 0;

    // Perform slower CO2/BMP/RTC/SD/serial operations once per second.
    maintenance_1Hz();
  }
}
