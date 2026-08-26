/////////////////////////////////////////////////////////////
//            WATER QUALITY MONITORING SYSTEM              //
//                ESP32-S3 + LoRa RAK3172                  //
//             --------------------------------            //
//     Sensors: Nitrate, pH, Temperature, Turbidity,       //
//                     Conductivity                        //
/////////////////////////////////////////////////////////////

//
// Connections
// ESP32-S3 DevKit :
//
//                         Nitrate ISE / Interface
// GPIO_NITRATE  --------- Signal
//
//                         Analog pH Sensor
// GPIO_PH       --------- Signal
//
//                         DS18B20 Temperature Sensor
// GPIO_TEMP     --------- Data
//
//                         Gravity Analog Turbidity Sensor
// GPIO_TURBIDITY -------- Signal
//
//                         Conductivity Sensor / Interface
// GPIO_CONDUCTIVITY ----- Signal
//
//                         LoRa RAK3172
// ESP32 TX -------------- RAK3172 RX
// ESP32 RX -------------- RAK3172 TX
// GND ------------------- GND
//
// NOTE:
// The GPIO numbers below are placeholders.
// Change them according to your actual ESP32-S3 wiring.
//

// ---------------------------------------------------------
// Include the required libraries.
// ---------------------------------------------------------

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------------------------------------------------
// Define the water quality sensor pins.
// ---------------------------------------------------------

#define NITRATE_SENSOR_PIN       1
#define pH_sensor               2
#define TURBIDITY_SENSOR_PIN    4
#define CONDUCTIVITY_SENSOR_PIN 5

// DS18B20 temperature sensor
#define ONE_WIRE_BUS             6

// ---------------------------------------------------------
// Define LoRa RAK3172 UART pins.
// ---------------------------------------------------------

#define LORA_RX_PIN              18
#define LORA_TX_PIN              17

#define LORA_BAUDRATE            115200

// Create LoRa serial instance.
HardwareSerial LoRaSerial(1);

// ---------------------------------------------------------
// Initialize the DS18B20 temperature sensor.
// ---------------------------------------------------------

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// ---------------------------------------------------------
// Define timers for water quality sensors.
// ---------------------------------------------------------

unsigned long read_timer, data_timer;

// ---------------------------------------------------------
// Define ADC settings for ESP32-S3.
// ---------------------------------------------------------

#define ADC_MAX_VALUE 4095.0
#define ADC_VOLTAGE   3.3

// ---------------------------------------------------------
// Define the pH sensor settings.
// ---------------------------------------------------------

#define pH_offset              0.19
#define pH_voltage_calibration 2.85

// ---------------------------------------------------------
// Define turbidity sensor settings.
// ---------------------------------------------------------

#define turbidity_calibration 0.65

// ---------------------------------------------------------
// Define general nitrate sensor settings.
// ---------------------------------------------------------

// These values are intentionally kept general.
// They MUST be calibrated when the actual nitrate sensor
// and its interface are finalized.

#define NITRATE_VOLTAGE_MIN    0.0
#define NITRATE_VALUE_MIN      0.0

#define NITRATE_VOLTAGE_MAX    3.3
#define NITRATE_VALUE_MAX      100.0

// ---------------------------------------------------------
// Define general conductivity sensor settings.
// ---------------------------------------------------------

// These values are intentionally kept general.
// They MUST be calibrated according to the actual
// conductivity sensor/interface.

#define CONDUCTIVITY_VOLTAGE_MIN  0.0
#define CONDUCTIVITY_VALUE_MIN    0.0

#define CONDUCTIVITY_VOLTAGE_MAX  3.3
#define CONDUCTIVITY_VALUE_MAX    2000.0

// ---------------------------------------------------------
// Define the data holders.
// ---------------------------------------------------------

float nitrate_value;
float pH_value;
float temperature;
float turbidity_value;
float NTU;
float conductivity_value;

// ---------------------------------------------------------
// Setup
// ---------------------------------------------------------

void setup(){

  Serial.begin(115200);

  // Initialize analog sensor pins.
  pinMode(NITRATE_SENSOR_PIN, INPUT);
  pinMode(pH_sensor, INPUT);
  pinMode(TURBIDITY_SENSOR_PIN, INPUT);
  pinMode(CONDUCTIVITY_SENSOR_PIN, INPUT);

  // Initialize the DS18B20 temperature sensor.
  DS18B20.begin();

  // Initialize LoRa RAK3172.
  LoRaSerial.begin(
    LORA_BAUDRATE,
    SERIAL_8N1,
    LORA_RX_PIN,
    LORA_TX_PIN
  );

  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println(" Water Quality Monitoring System");
  Serial.println(" ESP32-S3 + LoRa RAK3172");
  Serial.println("=================================");
  Serial.println();

  Serial.println("Sensors:");
  Serial.println("Nitrate");
  Serial.println("pH");
  Serial.println("Temperature");
  Serial.println("Turbidity");
  Serial.println("Conductivity");
  Serial.println();

  // Update timers.
  read_timer = millis();
  data_timer = millis();

}

// ---------------------------------------------------------
// Main Loop
// ---------------------------------------------------------

void loop(){

  // -------------------------------------------------------
  // Read sensor values periodically.
  // -------------------------------------------------------

  if(millis() - read_timer > 100){

    // Read nitrate measurement.
    nitrate_value = readNitrate();

    // Read pH measurement.
    pH_value = readPH();

    // Read temperature measurement.
    temperature = readTemperature();

    // Read turbidity measurement.
    NTU = readTurbidity();

    // Read conductivity measurement.
    conductivity_value = readConductivity();

    // Update timer.
    read_timer = millis();
  }

  // -------------------------------------------------------
  // Display / transmit accurate measurements.
  // -------------------------------------------------------

  if(millis() - data_timer > 1000){

    Serial.println("---------------------------------");

    Serial.print("Nitrate: ");
    Serial.print(nitrate_value);
    Serial.println(" mg/L");

    Serial.print("pH: ");
    Serial.println(pH_value);

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Turbidity: ");
    Serial.print(NTU);
    Serial.println(" NTU");

    Serial.print("Conductivity: ");
    Serial.print(conductivity_value);
    Serial.println(" uS/cm");

    Serial.println("---------------------------------");
    Serial.println();

    // Transmit the sensor data through LoRa.
    sendLoRaData();

    // Update timer.
    data_timer = millis();
  }

  // Display sensor measurements.
  show_sensor_measurements();

}

// =========================================================
// SENSOR FUNCTIONS
// =========================================================

// ---------------------------------------------------------
// Read Nitrate
// ---------------------------------------------------------

float readNitrate(){

  int rawValue = analogRead(NITRATE_SENSOR_PIN);

  float voltage =
    ((float)rawValue / ADC_MAX_VALUE) * ADC_VOLTAGE;

  /*
     GENERAL PLACEHOLDER CALIBRATION

     This converts the sensor voltage linearly into a
     nitrate concentration.

     IMPORTANT:
     This is NOT the final scientific calibration.

     Once the actual nitrate sensor/interface is known,
     replace this equation with its calibration equation.
  */

  float nitrate =
    NITRATE_VALUE_MIN +
    ((voltage - NITRATE_VOLTAGE_MIN) /
    (NITRATE_VOLTAGE_MAX - NITRATE_VOLTAGE_MIN)) *
    (NITRATE_VALUE_MAX - NITRATE_VALUE_MIN);

  if(nitrate < 0)
    nitrate = 0;

  return nitrate;
}

// ---------------------------------------------------------
// Read pH
// ---------------------------------------------------------

float readPH(){

  int rawValue = analogRead(pH_sensor);

  float pH_output =
    ((float)rawValue / ADC_MAX_VALUE) * ADC_VOLTAGE;

  /*
     Based on the calibration structure of the reference
     code.

     Original reference:
     pH_value = 3.5 * pH_output + pH_offset

     and then:
     pH_r_value = pH_value + pH_voltage_calibration
  */

  float pH =
    3.5 * pH_output +
    pH_offset +
    pH_voltage_calibration;

  return pH;
}

// ---------------------------------------------------------
// Read Temperature
// ---------------------------------------------------------

float readTemperature(){

  DS18B20.requestTemperatures();

  float temperatureValue =
    DS18B20.getTempCByIndex(0);

  return temperatureValue;
}

// ---------------------------------------------------------
// Read Turbidity
// ---------------------------------------------------------

float readTurbidity(){

  int rawValue =
    analogRead(TURBIDITY_SENSOR_PIN);

  float voltage =
    ((float)rawValue / ADC_MAX_VALUE) * ADC_VOLTAGE;

  /*
     General turbidity calculation.

     The original reference used a polynomial based on
     the turbidity sensor voltage:

     NTU = -(1120.4 * V^2)
           + (5742.3 * V)
           - 4352.9

     The same general calibration approach is retained.
  */

  float turbidity =
    -(1120.4 * voltage * voltage) +
    (5742.3 * voltage) -
    4352.9;

  // Apply reference calibration offset.
  turbidity = turbidity + turbidity_calibration;

  // Prevent negative turbidity values.
  if(turbidity < 0)
    turbidity = 0;

  return turbidity;
}

// ---------------------------------------------------------
// Read Conductivity
// ---------------------------------------------------------

float readConductivity(){

  int rawValue =
    analogRead(CONDUCTIVITY_SENSOR_PIN);

  float voltage =
    ((float)rawValue / ADC_MAX_VALUE) * ADC_VOLTAGE;

  /*
     GENERAL PLACEHOLDER CALIBRATION

     This is a temporary linear conversion.

     Once the actual conductivity module is selected,
     replace this with the sensor's actual calibration
     equation.

     Output unit:
     uS/cm
  */

  float conductivity =
    CONDUCTIVITY_VALUE_MIN +
    ((voltage - CONDUCTIVITY_VOLTAGE_MIN) /
    (CONDUCTIVITY_VOLTAGE_MAX - CONDUCTIVITY_VOLTAGE_MIN)) *
    (CONDUCTIVITY_VALUE_MAX - CONDUCTIVITY_VALUE_MIN);

  if(conductivity < 0)
    conductivity = 0;

  return conductivity;
}

// =========================================================
// DISPLAY SENSOR MEASUREMENTS
// =========================================================

void show_sensor_measurements(){

  /*
     No OLED is used here because the current circuit
     shows the authority desktop as the final display.

     Sensor values are therefore sent through Serial
     and LoRa.
  */

}

// =========================================================
// LORA DATA TRANSMISSION
// =========================================================

void sendLoRaData(){

  /*
     Create one compact data packet.

     Example:

     Nitrate=25.4,
     pH=7.12,
     Temp=27.3,
     Turbidity=12.5,
     Conductivity=630.2
  */

  String dataPacket = "";

  dataPacket += "Nitrate=";
  dataPacket += String(nitrate_value, 2);

  dataPacket += ",pH=";
  dataPacket += String(pH_value, 2);

  dataPacket += ",Temp=";
  dataPacket += String(temperature, 2);

  dataPacket += ",Turbidity=";
  dataPacket += String(NTU, 2);

  dataPacket += ",Conductivity=";
  dataPacket += String(conductivity_value, 2);

  // Display packet on Serial Monitor.
  Serial.println("LoRa Packet:");
  Serial.println(dataPacket);

  /*
     RAK3172 LoRa transmission command goes here.

     The exact command depends on whether the RAK3172
     is configured for LoRaWAN or LoRa P2P mode.

     For now the packet is prepared and sent to the
     RAK3172 UART.

     Configure the RAK3172 communication mode before
     enabling the actual AT transmission command.
  */

  LoRaSerial.println(dataPacket);
}
