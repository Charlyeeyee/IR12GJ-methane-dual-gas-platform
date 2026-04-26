#include <Wire.h>        // Provides I2C communication support for the ADS1115 ADC
#include <ADS1X15.h>     // Library for interfacing with ADS1015/ADS1115 ADC devices
#include <TimerOne.h>    // Library used to generate a precise timer interrupt for lamp chopping

ADS1115 ads;             // Create an ADS1115 ADC object

// -------------------- Hardware pin definitions --------------------
const int lampPin = 3;   // Arduino digital pin used to drive the IR12GJ lamp switching circuit

// -------------------- Interrupt flags --------------------
volatile bool lampState = false;        // Stores the current lamp state; false = OFF, true = ON
volatile bool RisingEdgeFlag = false;   // Set by the interrupt when the lamp switches ON
volatile bool FallingEdgeFlag = false;  // Set by the interrupt when the lamp switches OFF

// -------------------- ADS1115 channel assignments --------------------
const int sensor_temp = 0;  // ADS1115 channel connected to the IR12GJ internal temperature sensor
const int PCB_temp    = 1;  // ADS1115 channel reserved for PCB temperature measurement
const int ACT         = 3;  // ADS1115 channel connected to the active detector signal
const int REF         = 2;  // ADS1115 channel connected to the reference detector signal

// -------------------- Measurement cycle control --------------------
bool measureAct = true;     // Alternates between ACT and REF measurement cycles

// -------------------- Calibration constants --------------------
float zero  = 0.99790;      // Zero calibration value: ACT/REF ratio in zero methane condition
float Tcal  = 22;           // Calibration temperature in °C
float alpha = 0.000277;     // Alpha temperature-compensation coefficient for zero/baseline shift
float beta  = -0.106;       // Beta temperature-compensation coefficient; updated depending on temperature
float span  = 0.36154;      // Span value determined from pure-methane exposure
float a     = 0.19393;      // Fitted methane linearisation coefficient a
float n     = 0.73034;      // Fitted methane linearisation coefficient n

// -------------------- Timer interrupt routine --------------------
void onTickISR() {

  lampState = !lampState;              // Toggle the lamp state at every timer interrupt
  digitalWrite(lampPin, lampState);    // Apply the new lamp state to the lamp-drive pin

  if (lampState) {                     // If the lamp has just switched ON
    RisingEdgeFlag  = true;            // Signal the main loop to perform rising-edge sampling
    FallingEdgeFlag = false;           // Clear the falling-edge flag
  } else {                             // If the lamp has just switched OFF
    RisingEdgeFlag  = false;           // Clear the rising-edge flag
    FallingEdgeFlag = true;            // Signal the main loop to perform falling-edge sampling
  }
}

// -------------------- Rising-edge peak sampling --------------------
float intenseRisingSampling(int channel) {

  ads.setMode(ADS1X15_MODE_CONTINUOUS);  // Set ADS1115 to continuous conversion mode
  ads.requestADC(channel);               // Start continuous conversion on the selected ADC channel
  (void)ads.getValue();                  // Discard the first sample after channel switching

  float sampleMax = -32768;              // Initialise maximum value to a very low number

  for (int i = 0; i < 20; i++) {         // Collect 20 samples after the lamp rising edge
    delayMicroseconds(1200);             // Wait between samples to match the ADS1115 conversion timing
    int16_t counts = ads.getValue();     // Read the latest ADC conversion result in counts
    float value = ads.toVoltage(counts); // Convert ADC counts to voltage
    if (value > sampleMax) sampleMax = value; // Update the maximum voltage if this sample is larger
  }

  return sampleMax;                      // Return the measured peak voltage after the rising edge
}

// -------------------- Falling-edge trough sampling --------------------
float intenseFallingSampling(int channel) {

  ads.setMode(ADS1X15_MODE_CONTINUOUS);  // Set ADS1115 to continuous conversion mode
  ads.requestADC(channel);               // Start continuous conversion on the selected ADC channel
  (void)ads.getValue();                  // Discard the first sample after channel switching

  float sampleMin = 32767;               // Initialise minimum value to a very high number

  for (int i = 0; i < 20; i++) {         // Collect 20 samples after the lamp falling edge
    delayMicroseconds(1200);             // Wait between samples to match the ADS1115 conversion timing
    int16_t counts = ads.getValue();     // Read the latest ADC conversion result in counts
    float value = ads.toVoltage(counts); // Convert ADC counts to voltage
    if (value < sampleMin) sampleMin = value; // Update the minimum voltage if this sample is smaller
  }

  return sampleMin;                      // Return the measured trough voltage after the falling edge
}

// -------------------- Initial setup --------------------
void setup() {

  pinMode(lampPin, OUTPUT);              // Configure the lamp-drive pin as a digital output
  digitalWrite(lampPin, LOW);            // Ensure the lamp starts in the OFF state

  Wire.begin();                          // Initialise the I2C bus
  Wire.setClock(400000);                 // Set I2C clock speed to 400 kHz for faster ADC communication

  ads.begin();                           // Initialise communication with the ADS1115 ADC
  ads.setGain(1);                        // Set ADC programmable gain setting
  ads.setDataRate(7);                    // Set ADS1115 data rate to its maximum setting, 860 SPS

  Timer1.initialize(125000);             // Configure Timer1 interrupt period to 125 ms
                                         // 125 ms half-period gives a 4 Hz lamp square wave

  Timer1.attachInterrupt(onTickISR);     // Attach the lamp toggle interrupt routine to Timer1

  Serial.begin(115200);                  // Start serial communication at 115200 baud
}

// -------------------- Peak and trough storage variables --------------------
float act_max = 0, act_min = 0;           // Store latest active-channel maximum and minimum voltages
float ref_max = 0, ref_min = 0;           // Store latest reference-channel maximum and minimum voltages

float act_last_max = NAN, act_last_min = NAN; // Store previous active-channel peak and trough values
float ref_last_max = NAN, ref_last_min = NAN; // Store previous reference-channel peak and trough values

unsigned long lastPrintTime = 0;          // Stores the time of the previous 1-second serial output

// -------------------- Averaging accumulators --------------------
float sumACT = 0;                         // Accumulates active-channel peak-to-peak amplitudes
float sumREF = 0;                         // Accumulates reference-channel peak-to-peak amplitudes
float sumSensorTemp = 0;                  // Accumulates temperature readings

unsigned long countACT = 0;               // Counts active-channel amplitude samples
unsigned long countREF = 0;               // Counts reference-channel amplitude samples
unsigned long countCycles = 0;            // Counts total lamp cycles used for temperature averaging

// -------------------- Main loop --------------------
void loop() {

  if (RisingEdgeFlag) {                   // Check whether the lamp has just switched ON
    RisingEdgeFlag = false;               // Clear the rising-edge flag after detecting it

    if (measureAct)                       // If the current cycle is assigned to the active channel
      act_max = intenseRisingSampling(ACT); // Measure the active-channel rising-edge peak
    else                                  // Otherwise, the current cycle is assigned to the reference channel
      ref_max = intenseRisingSampling(REF); // Measure the reference-channel rising-edge peak
  }

  if (FallingEdgeFlag) {                  // Check whether the lamp has just switched OFF
    FallingEdgeFlag = false;              // Clear the falling-edge flag after detecting it

    if (measureAct)                       // If the current cycle is assigned to the active channel
      act_min = intenseFallingSampling(ACT); // Measure the active-channel falling-edge trough
    else                                  // Otherwise, the current cycle is assigned to the reference channel
      ref_min = intenseFallingSampling(REF); // Measure the reference-channel falling-edge trough

    if (measureAct) {                     // If this completed an active-channel measurement cycle
      act_last_max = act_max;             // Store the latest active-channel peak
      act_last_min = act_min;             // Store the latest active-channel trough
    } else {                              // If this completed a reference-channel measurement cycle
      ref_last_max = ref_max;             // Store the latest reference-channel peak
      ref_last_min = ref_min;             // Store the latest reference-channel trough
    }

    float ACT_amp = act_max - act_min;    // Calculate active-channel peak-to-peak amplitude
    float REF_amp = ref_max - ref_min;    // Calculate reference-channel peak-to-peak amplitude

    int16_t st_counts = ads.readADC(sensor_temp);       // Read raw ADC counts from the temperature sensor channel
    float Sensor_temp_volts = ads.toVoltage(st_counts); // Convert temperature sensor ADC counts to voltage

    float Sensor_temp = (Sensor_temp_volts - 0.424) / 0.00625;
                                         // Convert IR12GJ IC temperature-sensor voltage to temperature in °C

    if (measureAct) {                    // If this was an active-channel cycle
      sumACT += ACT_amp;                 // Add active amplitude to the 1-second accumulator
      countACT++;                        // Increment active-channel sample count
    } else {                             // If this was a reference-channel cycle
      sumREF += REF_amp;                 // Add reference amplitude to the 1-second accumulator
      countREF++;                        // Increment reference-channel sample count
    }

    sumSensorTemp += Sensor_temp;         // Add current temperature reading to the accumulator
    countCycles++;                        // Increment total completed measurement cycles

    measureAct = !measureAct;             // Alternate between ACT and REF measurement on the next cycle
  }

  if (millis() - lastPrintTime >= 1000) { // Check whether one second has passed since the last output

    lastPrintTime = millis();             // Update the output timer

    if (countCycles > 0) {                // Only calculate results if at least one cycle was completed

      float avgACT = (countACT > 0) ? (sumACT / countACT) : 0.0f;
                                         // Calculate 1-second average active-channel amplitude

      float avgREF = (countREF > 0) ? (sumREF / countREF) : 0.0f;
                                         // Calculate 1-second average reference-channel amplitude

      float avgSensorTemp = sumSensorTemp / countCycles;
                                         // Calculate 1-second average sensor temperature

      beta = (avgSensorTemp >= Tcal) ? -0.106 : -0.137;
                                         // Select positive or negative beta coefficient based on temperature

      float NR = avgACT / (zero * avgREF);
                                         // Calculate normalised ratio from ACT, REF and zero calibration

      float NRcomp = NR * (1.0f + alpha * (avgSensorTemp - Tcal));
                                         // Apply alpha temperature compensation to the normalised ratio

      float Spancomp = span + beta * ((avgSensorTemp - Tcal) / Tcal);
                                         // Apply beta temperature compensation to the span value

      float arg = -log(1.0f - ((1.0f - NRcomp) / Spancomp));
                                         // Rearranged methane concentration equation numerator

      if (arg < 0) arg = 0;               // Prevent invalid negative values from being used in the power calculation

      float base = arg / a;               // Divide by linearisation coefficient a
      float c = pow(base, 1.0f / n);      // Calculate methane concentration in % v/v
      float c_ppm = c * 10000.0f;         // Convert methane concentration from % v/v to ppm

      static bool headerPrinted = false;  // Tracks whether the CSV header has already been printed

      if (!headerPrinted) {               // If the header has not yet been printed
        Serial.println("ACT_pp,REF_pp,Temp_C,CH4_ppm"); // Print CSV column headings
        headerPrinted = true;             // Prevent the header from being printed again
      }

      Serial.print(avgACT, 6);            // Print averaged active-channel amplitude with 6 decimal places
      Serial.print(",");                 // Print CSV delimiter
      Serial.print(avgREF, 6);            // Print averaged reference-channel amplitude with 6 decimal places
      Serial.print(",");                 // Print CSV delimiter
      Serial.print(avgSensorTemp, 2);     // Print averaged sensor temperature with 2 decimal places
      Serial.print(",");                 // Print CSV delimiter
      Serial.println(c_ppm, 2);           // Print methane concentration in ppm with 2 decimal places
    }

    sumACT = 0;                           // Reset active-channel accumulator
    sumREF = 0;                           // Reset reference-channel accumulator
    sumSensorTemp = 0;                    // Reset temperature accumulator

    countACT = 0;                         // Reset active-channel sample counter
    countREF = 0;                         // Reset reference-channel sample counter
    countCycles = 0;                      // Reset total cycle counter
  }
}
