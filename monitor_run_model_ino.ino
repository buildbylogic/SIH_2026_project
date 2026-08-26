/////////////////////////////////////////////////////////////
//           WATER QUALITY MONITORING SYSTEM                  //
//                       TinyML MODEL + LoRa RAK3172                      //
//                               ESP32-S3 DevKit                                    //
//                          --------------------------------                        //
//             Sensors: Nitrate, pH, Temperature, Turbidity,           //
//                                      Conductivity                                    //
//                                                                                            //
//                       TinyML Output Classes:                                  //
//                          Clean / Risky / Polluted                               //
/////////////////////////////////////////////////////////////

//
// IMPORTANT:
//
// The Neuton model used with this code MUST be trained
// using these 5 inputs in exactly this order:
//
// 1. Nitrate
// 2. pH
// 3. Temperature
// 4. Turbidity
// 5. Conductivity
//
// The generated "neuton.h" file must therefore correspond
// to this 5-input model.
//

// ---------------------------------------------------------
// Connections
// ---------------------------------------------------------
//
// ESP32-S3:
//
// Nitrate sensor/interface
// GPIO_NITRATE       -------- Signal
//
// Analog pH Sensor
// GPIO_PH            -------- Signal
//
// DS18B20
// GPIO_TEMP          -------- Data
//
// Gravity Analog Turbidity Sensor
// GPIO_TURBIDITY     -------- Signal
//
// Conductivity sensor/interface
// GPIO_CONDUCTIVITY  -------- Signal
//
// LoRa RAK3172
// ESP32 TX           -------- RAK3172 RX
// ESP32 RX           -------- RAK3172 TX
// GND                -------- GND
//
// NOTE:
// GPIO numbers below are placeholders.
// Replace them with your actual wiring.
//

// ---------------------------------------------------------
// Include the required libraries.
// ---------------------------------------------------------

#include <Arduino.h>
#include "neuton.h"
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

HardwareSerial LoRaSerial(1);

// ---------------------------------------------------------
// Initialize the DS18B20 temperature sensor.
// ---------------------------------------------------------

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// ---------------------------------------------------------
// Define timers for water quality sensors.
// ---------------------------------------------------------

unsigned long read_timer, data_timer, inference_timer;

// ---------------------------------------------------------
// ESP32-S3 ADC settings.
// ---------------------------------------------------------

#define ADC_MAX_VALUE 4095.0
#define ADC_VOLTAGE   3.3

// ---------------------------------------------------------
// Define the pH sensor settings.
// ---------------------------------------------------------

#define pH_offset               0.19
#define pH_voltage_calibration 2.85

// ---------------------------------------------------------
// Define turbidity sensor settings.
// ---------------------------------------------------------

#define turbidity_calibration  0.65

// ---------------------------------------------------------
// General nitrate settings.
//
// These are placeholders until the actual nitrate
// sensor/interface and calibration are finalized.
// ---------------------------------------------------------

#define NITRATE_VOLTAGE_MIN    0.0
#define NITRATE_VOLTAGE_MAX    3.3

#define NITRATE_VALUE_MIN      0.0
#define NITRATE_VALUE_MAX      100.0

// ---------------------------------------------------------
// General conductivity settings.
//
// These are placeholders until the actual conductivity
// sensor/interface and calibration are finalized.
// ---------------------------------------------------------

#define CONDUCTIVITY_VOLTAGE_MIN  0.0
#define CONDUCTIVITY_VOLTAGE_MAX  3.3

#define CONDUCTIVITY_VALUE_MIN    0.0
#define CONDUCTIVITY_VALUE_MAX    2000.0

// ---------------------------------------------------------
// Define the water pollution level class names.
//
// These are retained from the reference model.
// ---------------------------------------------------------

String classes[] = {
  "Clean",
  "Risky",
  "Polluted"
};

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
// Store the model prediction.
// ---------------------------------------------------------

uint16_t predictedClass;

float* probabilities;

// ---------------------------------------------------------
// SETUP
// ---------------------------------------------------------

void setup(){

  Serial.begin(115200);

  // Initialize analog sensor pins.
  pinMode(NITRATE_SENSOR_PIN, INPUT);
  pinMode(pH_sensor, INPUT);
  pinMode(TURBIDITY_SENSOR_PIN, INPUT);
  pinMode(CONDUCTIVITY_SENSOR_PIN, INPUT);

  // Initialize DS18B20.
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
  Serial.println("========================================");
  Serial.println(" WATER QUALITY MONITORING SYSTEM");
  Serial.println(" ESP32-S3 + Neuton TinyML + LoRa");
  Serial.println("========================================");
  Serial.println();

  Serial.println("Sensors:");
  Serial.println("1. Nitrate");
  Serial.println("2. pH");
  Serial.println("3. Temperature");
  Serial.println("4. Turbidity");
  Serial.println("5. Conductivity");
  Serial.println();

  Serial.println("TinyML Classes:");
  Serial.println("0 = Clean");
  Serial.println("1 = Risky");
  Serial.println("2 = Polluted");
  Serial.println();

  // Initialize timers.
  read_timer = millis();
  data_timer = millis();
  inference_timer = millis();

}

// ---------------------------------------------------------
// MAIN LOOP
// ---------------------------------------------------------

void loop(){

  // -------------------------------------------------------
  // Calculate sensor measurements every 20 ms.
  // -------------------------------------------------------

  if(millis() - read_timer > 20){

    nitrate_value = readNitrate();

    pH_value = readPH();

    temperature = readTemperature();

    NTU = readTurbidity();

    conductivity_value = readConductivity();

    // Update timer.
    read_timer = millis();

  }

  // -------------------------------------------------------
  // Display accurate measurements every 800 ms.
  // -------------------------------------------------------

  if(millis() - data_timer > 800){

    Serial.println("----------------------------------------");

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

    Serial.println("----------------------------------------");
    Serial.println();

    data_timer = millis();

  }

  // -------------------------------------------------------
  // Run TinyML inference every 2 seconds.
  // -------------------------------------------------------

  if(millis() - inference_timer > 2000){

    run_inference_to_make_predictions();

    inference_timer = millis();

  }

}

// =========================================================
// SENSOR FUNCTIONS
// =========================================================

// ---------------------------------------------------------
// Read Nitrate
// ---------------------------------------------------------

float readNitrate(){

  int rawValue =
    analogRead(NITRATE_SENSOR_PIN);

  float voltage =
    ((float)rawValue / ADC_MAX_VALUE) * ADC_VOLTAGE;

  /*
     GENERAL PLACEHOLDER CALIBRATION

     Replace this equation once the actual nitrate
     sensor/interface is finalized.

     Current placeholder:
     0 - 3.3 V -> 0 - 100 mg/L
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

  int rawValue =
    analogRead(pH_sensor);

  float pH_output =
    ((float)rawValue / ADC_MAX_VALUE) *
    ADC_VOLTAGE;

  /*
     Reference pH calculation retained in structure.

     Original:
     pH_value = 3.5 * pH_output + pH_offset

     Then calibration:
     pH_r_value = pH_value +
                  pH_voltage_calibration
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
    ((float)rawValue / ADC_MAX_VALUE) *
    ADC_VOLTAGE;

  /*
     Turbidity calibration retained from the reference
     approach.

     Original polynomial:

     NTU = -(1120.4 * V²)
           + (5742.3 * V)
           - 4352.9

     The original code divided the result by 1000.
  */

  float turbidity =
    -(1120.4 * voltage * voltage) +
    (5742.3 * voltage) -
    4352.9;

  turbidity =
    turbidity / 1000.0;

  turbidity =
    turbidity + turbidity_calibration;

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
    ((float)rawValue / ADC_MAX_VALUE) *
    ADC_VOLTAGE;

  /*
     GENERAL PLACEHOLDER CALIBRATION

     Current placeholder:
     0 - 3.3 V -> 0 - 2000 uS/cm

     Replace with the actual conductivity sensor
     calibration equation later.
  */

  float conductivity =
    CONDUCTIVITY_VALUE_MIN +
    ((voltage - CONDUCTIVITY_VOLTAGE_MIN) /
    (CONDUCTIVITY_VOLTAGE_MAX -
     CONDUCTIVITY_VOLTAGE_MIN)) *
    (CONDUCTIVITY_VALUE_MAX -
     CONDUCTIVITY_VALUE_MIN);

  if(conductivity < 0)
    conductivity = 0;

  return conductivity;

}

// =========================================================
// NEUTON TINYML INFERENCE
// =========================================================

void run_inference_to_make_predictions(){

  // -------------------------------------------------------
  // Create the input array with the water quality
  // sensor measurements.
  //
  // IMPORTANT:
  // The order MUST be identical to the order used while
  // training the Neuton model.
  // -------------------------------------------------------

  float input_array[] = {

    nitrate_value,
    pH_value,
    temperature,
    NTU,
    conductivity_value

  };

  Serial.println();
  Serial.println("========================================");
  Serial.println("Running Neuton TinyML Inference...");
  Serial.println("========================================");

  Serial.println("Model Inputs:");

  Serial.print("Nitrate       = ");
  Serial.println(input_array[0]);

  Serial.print("pH            = ");
  Serial.println(input_array[1]);

  Serial.print("Temperature   = ");
  Serial.println(input_array[2]);

  Serial.print("Turbidity     = ");
  Serial.println(input_array[3]);

  Serial.print("Conductivity  = ");
  Serial.println(input_array[4]);

  // -------------------------------------------------------
  // Run inference.
  //
  // This is the original Neuton command structure.
  // -------------------------------------------------------

  if(neuton_model_set_inputs(input_array) == 0){

    // -----------------------------------------------------
    // Read predicted output.
    // -----------------------------------------------------

    if(neuton_model_run_inference(
      &predictedClass,
      &probabilities
    ) == 0){

      // ---------------------------------------------------
      // Display prediction.
      // ---------------------------------------------------

      Serial.println();
      Serial.println("******** MODEL RESULT ********");

      if(predictedClass < 3){

        Serial.print("Predicted Class: ");
        Serial.println(classes[predictedClass]);

      }else{

        Serial.println("Predicted Class: UNKNOWN");

      }

      // ---------------------------------------------------
      // Display probabilities.
      // ---------------------------------------------------

      Serial.println();
      Serial.println("Class Probabilities:");

      for(int i = 0; i < 3; i++){

        Serial.print(classes[i]);
        Serial.print(": ");

        Serial.print(
          probabilities[i] * 100.0,
          2
        );

        Serial.println(" %");

      }

      Serial.println("*******************************");
      Serial.println();

      // ---------------------------------------------------
      // Send result through LoRa.
      // ---------------------------------------------------

      sendLoRaPrediction();

    }else{

      Serial.println(
        "ERROR: Neuton inference failed!"
      );

    }

  }else{

    Serial.println(
      "ERROR: Neuton model input failed!"
    );

  }

}

// =========================================================
// SEND MODEL RESULT THROUGH LORA
// =========================================================

void sendLoRaPrediction(){

  if(predictedClass >= 3)
    return;

  String dataPacket = "";

  // Sensor measurements.
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

  // TinyML prediction.
  dataPacket += ",Prediction=";
  dataPacket += classes[predictedClass];

  // Probability of predicted class.
  dataPacket += ",Confidence=";
  dataPacket += String(
    probabilities[predictedClass] * 100.0,
    2
  );

  // Send packet to RAK3172 UART.
  LoRaSerial.println(dataPacket);

  // Also display on Serial Monitor.
  Serial.println("LoRa Packet:");
  Serial.println(dataPacket);
  Serial.println();

}
