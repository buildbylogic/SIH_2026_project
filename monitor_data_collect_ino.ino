       /////////////////////////////////////////////
      //    GSM & SMS Enabled Water Pollution    //
     //         Monitor w/ Neuton TinyML        //
    //             ---------------             //
   //          (Arduino MKR GSM 1400)         //
  //             by Kutluhan Aktar           //
 //                                         //
/////////////////////////////////////////////

//
// Via MKR GSM 1400, collate water quality data from resources over GPRS to train a Neuton model and run the model to transmit output via SMS.
//
// For more information:
// https://www.theamplituhedron.com/projects/GSM_SMS_Enabled_Water_Pollution_Monitor_w_Neuton_TinyML
//
//
// Connections
// Arduino MKR GSM 1400 :
//                                DFRobot Analog ORP Sensor
// A1  --------------------------- Signal
//                                DFRobot Analog pH Sensor Pro Kit
// A2  --------------------------- Signal
//                                DFRobot Analog TDS Sensor
// A3  --------------------------- Signal
//                                DFRobot Analog Turbidity Sensor
// A4  --------------------------- Signal
//                                DS18B20 Waterproof Temperature Sensor
// D1  --------------------------- Data
//                                SH1106 OLED Display (128x64)
// MOSI  ------------------------- SDA
// SCK   ------------------------- SCK
// D10   ------------------------- RST
// D11   ------------------------- DC
// D12   ------------------------- CS
//                                5mm Common Anode RGB LED
// D2  --------------------------- R
// D3  --------------------------- G
// D4  --------------------------- B  
//                                Control Button (1)
// D5  --------------------------- +
//                                Control Button (2)
// D6  --------------------------- +
//                                Control Button (3)
// D7  --------------------------- +


// Include the required libraries.
#include <MKRGSM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define the APN (Access Point Name) information:
// https://apn.how/
#define PINNUMBER     ""
#define GPRS_APN      "internet"
#define GPRS_LOGIN    ""
#define GPRS_PASSWORD ""

// Initialize the GSM and GPRS instances:
GSMSSLClient client;
GPRS gprs;
GSM gsmAccess;

// Define the URL, path, and port (for example, arduino.cc):
char server[] = "www.theamplituhedron.com";
String path = "/water_pollution_data_logger/";
int port = 443; // port 443 is the default for HTTPS

// Define the SH1106 screen settings:
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_MOSI     MOSI
#define OLED_CLK      SCK
#define OLED_DC       11
#define OLED_CS       12
#define OLED_RST      10

// Create the SH1106 OLED screen.
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

// Define monochrome graphics:
static const unsigned char PROGMEM _error [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x01, 0x80, 0x01, 0x80,
0x06, 0x00, 0x00, 0x60, 0x0C, 0x00, 0x00, 0x30, 0x08, 0x01, 0x80, 0x10, 0x10, 0x03, 0xC0, 0x08,
0x30, 0x02, 0x40, 0x0C, 0x20, 0x02, 0x40, 0x04, 0x60, 0x02, 0x40, 0x06, 0x40, 0x02, 0x40, 0x02,
0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02,
0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x03, 0xC0, 0x02, 0x40, 0x01, 0x80, 0x02,
0x40, 0x00, 0x00, 0x02, 0x60, 0x00, 0x00, 0x06, 0x20, 0x01, 0x80, 0x04, 0x30, 0x03, 0xC0, 0x0C,
0x10, 0x03, 0xC0, 0x08, 0x08, 0x01, 0x80, 0x10, 0x0C, 0x00, 0x00, 0x30, 0x06, 0x00, 0x00, 0x60,
0x01, 0x80, 0x01, 0x80, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM water [] = {
0x3F, 0xFF, 0xFF, 0xFC, 0x7F, 0xFF, 0xFF, 0xFE, 0x60, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x06,
0x70, 0x00, 0x00, 0x0E, 0x70, 0x00, 0x00, 0x0C, 0x30, 0x01, 0x80, 0x0C, 0x30, 0x01, 0x80, 0x0C,
0x30, 0x03, 0xC0, 0x0C, 0x30, 0x03, 0xC0, 0x0C, 0x38, 0x07, 0xE0, 0x1C, 0x38, 0x0F, 0xF0, 0x18,
0x18, 0x1F, 0xF8, 0x18, 0x18, 0x3F, 0xFC, 0x18, 0x18, 0x7F, 0xFE, 0x18, 0x18, 0xFF, 0xFE, 0x18,
0x18, 0xFF, 0xFF, 0x38, 0x1D, 0xFF, 0xFF, 0x30, 0x0D, 0xFF, 0xFF, 0xB0, 0x0D, 0xFF, 0xFF, 0xB0,
0x0D, 0xDF, 0xFF, 0xB0, 0x0D, 0xDF, 0xFF, 0xB0, 0x0D, 0xDF, 0xFF, 0x70, 0x0E, 0xEF, 0xFF, 0x60,
0x0E, 0xE7, 0xFF, 0x60, 0x06, 0x70, 0xFE, 0x60, 0x06, 0x3E, 0xFC, 0x60, 0x06, 0x1F, 0xF8, 0x60,
0x06, 0x07, 0xE0, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0xFF, 0xFF, 0xC0, 0x01, 0xFF, 0xFF, 0x80
};

const unsigned char source [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x40, 0x48, 0x00, 0x00, 0x00,
0x60, 0x84, 0x00, 0x00, 0x00, 0x41, 0x02, 0x00, 0x00, 0x00, 0x62, 0x31, 0x00, 0x00, 0x00, 0x44,
0x78, 0x80, 0x00, 0x00, 0x08, 0xFC, 0x40, 0x00, 0x00, 0x11, 0xFE, 0x20, 0x00, 0x00, 0x23, 0xFF,
0x10, 0x00, 0x00, 0x47, 0xFF, 0x88, 0x00, 0x00, 0x8F, 0xFF, 0xC4, 0x00, 0x01, 0x1F, 0xFF, 0xE2,
0x00, 0x06, 0x3F, 0xFF, 0xF1, 0x00, 0x00, 0x7F, 0xFF, 0xF0, 0x00, 0x00, 0x70, 0x3F, 0xF0, 0x00,
0x00, 0x60, 0x1F, 0xF0, 0x00, 0x00, 0x67, 0x9F, 0xF0, 0x00, 0x00, 0x67, 0x9F, 0x80, 0x00, 0x00,
0x67, 0x9F, 0x07, 0x00, 0x00, 0x67, 0x9E, 0x3F, 0xC0, 0x0E, 0x67, 0x9C, 0x78, 0xF0, 0x1E, 0x67,
0x98, 0xF0, 0xF0, 0x18, 0x67, 0x99, 0xF0, 0x78, 0x30, 0x00, 0x01, 0xE2, 0x78, 0x30, 0x00, 0x01,
0xE6, 0x7C, 0x30, 0x00, 0x01, 0xE7, 0x3C, 0x30, 0x00, 0x01, 0xCF, 0x3C, 0x30, 0x00, 0x01, 0xCF,
0x3C, 0x30, 0x07, 0x81, 0xCF, 0x38, 0x30, 0x1F, 0xC1, 0xC7, 0x38, 0x30, 0x1C, 0xE0, 0xE0, 0x30,
0x30, 0x30, 0x60, 0x70, 0x70, 0x30, 0x30, 0x30, 0x3F, 0xC0, 0x30, 0x30, 0x30, 0x0F, 0x00, 0x38,
0x70, 0x30, 0x00, 0x00, 0x1C, 0xE0, 0x00, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x00, 0x07, 0x80,
0xF8, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 
};


// Define timers for water quality sensors.
unsigned long read_timer, data_timer;

// Define the water quality sensor pins:  
#define orp_sensor A1
#define pH_sensor A2
#define tds_sensor A3
#define turbidity_sensor A4

// Define the ORP sensor settings:
#define orp_offset 21
#define orp_voltage 3.3
#define orp_voltage_calibration 95
#define orp_array_length 40
int orp_array_index = 0, orp_array[orp_array_length];

// Define the pH sensor settings:
#define pH_offset 0.19
#define pH_voltage 3.3
#define pH_voltage_calibration 2.85
#define pH_array_length 40
int pH_array_index = 0, pH_array[pH_array_length];

// Define the TDS sensor settings:
#define tds_voltage 3.3  
#define tds_array_length 30
int tds_array[tds_array_length], tds_array_temp[tds_array_length];
int tds_array_index = -1;

// Define the DS18B20 waterproof temperature sensor settings:
#define ONE_WIRE_BUS 1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// Define the turbidity sensor settings:
#define turbidity_calibration 0.65

// Define the control buttons: 
#define BUTTON_1   5
#define BUTTON_2   6
#define BUTTON_3   7

// Define the RGB pins:
#define redPin     2
#define greenPin   3
#define bluePin    4

// Define the data holders:
double orp_value, orp_r_value;
float pH_value, pH_r_value, tds_value, temperature, turbidity_value, NTU;

void setup(){
  Serial.begin(9600);

  pinMode(tds_sensor, INPUT);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  adjustColor(0,0,0);

  // Initialize the DS18B20 sensor.
  DS18B20.begin();
  
  // Initialize the SH1106 screen:
  display.begin(0, true);
  display.display();
  delay(1000);

  display.clearDisplay();   
  display.setTextSize(2); 
  display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(0,0);
  display.println("Water");
  display.println("Pollution");
  display.println("Monitor");
  display.display();
  delay(1000);

  // Start the modem and attach the Arduino MKR GSM 1400 to the GPRS network with the APN, login, and password variables.
  bool connected = false;
  // Uncomment to debug errors with AT commands.
  //MODEM.debug(); 
  while(!connected){
    if((gsmAccess.begin(PINNUMBER) == GSM_READY) && (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)){
      connected = true;
    }else{
      Serial.println("GSM Modem: Not connected!\n");
      err_msg();
      delay(1000);
    }
  }
  // After connecting to the GPRS network successfully:
  Serial.println("GSM Modem: Connected successfully to the GPRS network!\n");
  display.clearDisplay();   
  display.setTextSize(1); 
  display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(0,20);
  display.println("GSM Modem: Connected successfully to the GPRS network!");
  display.display();
  adjustColor(0,0,255);
  delay(2000);
  display.invertDisplay(true);
  delay(2000);
  display.invertDisplay(false);
  delay(2000);
  adjustColor(0,0,0);

  // Update timers:
  read_timer = millis(); data_timer = millis();
  
}

void loop(){
  if(millis() - read_timer > 20){
    // Calculate the ORP measurement every 20 milliseconds.
    orp_array[orp_array_index++] = analogRead(orp_sensor);
    if(orp_array_index == orp_array_length) orp_array_index = 0;
    orp_value = ((30*(double)orp_voltage*1000)-(75*avr_arr(orp_array, orp_array_length)*orp_voltage*1000/1024))/75-orp_offset;
    
    // Calculate the pH measurement every 20 milliseconds.
    pH_array[pH_array_index++] = analogRead(pH_sensor);
    if(pH_array_index == pH_array_length) pH_array_index = 0;
    float pH_output = avr_arr(pH_array, pH_array_length) * pH_voltage / 1024;
    pH_value = 3.5 * pH_output + pH_offset;

    // Calculate the TDS measurement every 20 milliseconds.
    tds_array[tds_array_index++] = analogRead(tds_sensor);
    if(tds_array_index == tds_array_length) tds_array_index = 0;
    
    // Update the timer.  
    read_timer = millis();
  }
  
  if(millis() - data_timer > 800){
    // Get the accurate ORP measurement every 800 milliseconds.
    orp_r_value = orp_value + orp_voltage_calibration;
    Serial.print("ORP: "); Serial.print((int)orp_r_value); Serial.println(" mV");
    
    // Get the accurate pH measurement every 800 milliseconds.
    pH_r_value = pH_value + pH_voltage_calibration;
    Serial.print("pH: "); Serial.println(pH_r_value);

    // Get the temperature value in Celsius. 
    DS18B20.requestTemperatures(); 
    temperature = DS18B20.getTempCByIndex(0);
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" C");

    // Get the accurate TDS measurement every 800 milliseconds.
    for(int i=0; i<tds_array_length; i++) tds_array_temp[i] = tds_array[i];
    float tds_average_voltage = getMedianNum(tds_array_temp, tds_array_length) * (float)tds_voltage / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensatedVoltage = tds_average_voltage / compensationCoefficient;
    tds_value = (133.42*compensatedVoltage*compensatedVoltage*compensatedVoltage - 255.86*compensatedVoltage*compensatedVoltage + 857.39*compensatedVoltage)*0.5;
    Serial.print("TDS: "); Serial.print(tds_value); Serial.println(" ppm");

    // Get the accurate turbidity measurement every 800 milliseconds.
    turbidity_value = analogRead(turbidity_value) * (3.3 / 1024.0) + turbidity_calibration;
    NTU = -(1120.4*turbidity_value*turbidity_value) + (5742.3*turbidity_value) - 4352.9;
    NTU = NTU / 1000;
    Serial.print("Turbidity: "); Serial.print(NTU); Serial.println(" NTU");

    // Update the timer.
    data_timer = millis();
    Serial.println("");
  }
  
  // Display the sensor measurements on the OLED screen.
  show_sensor_measurements();

  // Transmit the data packet to the PHP application with the selected pollution class:
  if(!digitalRead(BUTTON_1)) make_a_get_request("0"); 
  if(!digitalRead(BUTTON_2)) make_a_get_request("1"); 
  if(!digitalRead(BUTTON_3)) make_a_get_request("2"); 

}

void make_a_get_request(String pollution){
  if(client.connect(server, port)){
    Serial.println("GSM Modem: Connected to the server!");
    // Update the path to transfer the given data packet accurately:
    path = path + "?orp=" + String(int(orp_r_value)) + "&pH=" + String(pH_r_value) + "&tds=" + String(tds_value) + "&turbidity=" + String(NTU) + "&pollution=" + pollution;
    // Make an HTTPS request to the given server:
    client.print("GET ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    adjustColor(255,255,0); delay(500); adjustColor(0,0,0);
  }else{
    // If the GSM modem cannot connect to the given server:
    Serial.println("GSM Modem: Cannot connect to the server!\n");
    err_msg();
  }
  delay(2000);
  // If there is a response from the server:
  String response = "";
  while(client.available()) response += (char)client.read();
  Serial.println(response);
  // If the PHP application saves the transferred data packet to the given CSV file successfully:
  if(response && response.indexOf("The given data packet is added to") > 0){
    display.clearDisplay();   
    display.drawBitmap((SCREEN_WIDTH/2)-(40/2), 0, source, 40, 40, SH110X_WHITE);
    display.setTextSize(1); 
    display.setTextColor(SH110X_WHITE);
    display.setCursor(30,42);
    display.print("Data Saved!");
    display.setCursor(30,52);
    display.print("Pollution: "); display.print(pollution);
    display.display();
    adjustColor(0,255,0);
    delay(2000);
    display.invertDisplay(true);
    delay(2000);
    display.invertDisplay(false);
    delay(2000); 
    adjustColor(0,0,0);
  }else{
    Serial.println("GSM Modem: No response from the server!\n");
    err_msg();
  }
  // If the server is disconnected, stop the client:
  if(!client.available() && !client.connected()){
    Serial.println("GSM Modem: Disconnecting from the server!\n");
    client.stop();
  }
}

void show_sensor_measurements(){
  display.clearDisplay();   
  display.drawBitmap(SCREEN_WIDTH-32, (SCREEN_HEIGHT-32)/2, water, 32, 32, SH110X_WHITE);
  display.setTextSize(1); 
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,5);
  display.print("ORP: "); display.print(int(orp_r_value)); display.println("mV");
  display.print("pH: "); display.println(pH_r_value); display.println();
  display.print("Temp: "); display.print(temperature); display.println("*C\n");
  display.print("TDS: "); display.print(tds_value); display.println("ppm");
  display.print("Turbidity: "); display.print(turbidity_value); display.println("V");
  display.display();
  delay(100);
}

double avr_arr(int* arr, int number){
  int i, max, min;
  double avg;
  long amount=0;
  if(number<=0){ Serial.println("ORP Sensor Error: 0"); return 0; }
  if(number<5){
    for(i=0; i<number; i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){ min = arr[0];max=arr[1]; }
    else{ min = arr[1]; max = arr[0]; }
    for(i=2; i<number; i++){
      if(arr[i]<min){ amount+=min; min=arr[i];}
      else{
        if(arr[i]>max){ amount+=max; max=arr[i]; } 
        else{
          amount+=arr[i];
        }
      }
    }
    avg = (double)amount/(number-2);
  }
  return avg;
}

int getMedianNum(int bArray[], int iFilterLen){  
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++) bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++){
      if (bTab[i] > bTab[i + 1]){
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) bTemp = bTab[(iFilterLen - 1) / 2];
  else bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}

void err_msg(){
  // Show the error message on the SH1106 screen.
  display.clearDisplay();   
  display.drawBitmap(48, 0, _error, 32, 32, SH110X_WHITE);
  display.setTextSize(1); 
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,40); 
  display.println("Check the serial monitor to see the error!");
  display.display(); 
  adjustColor(255,0,0);
  delay(1000);
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000); 
  adjustColor(0,0,0);
}

void adjustColor(int r, int g, int b){
  analogWrite(redPin, (255-r));
  analogWrite(greenPin, (255-g));
  analogWrite(bluePin, (255-b));
}
