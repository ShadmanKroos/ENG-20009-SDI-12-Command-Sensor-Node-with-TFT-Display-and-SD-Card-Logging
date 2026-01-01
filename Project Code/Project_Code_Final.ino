#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <BH1750.h>
#include "SdFat.h"

#define TFT_CS    10
#define TFT_RST   6 
#define TFT_DC    7 

#define TFT_SCLK 13   
#define TFT_MOSI 11  

#define BUTTON_PIN1 2
#define BUTTON_PIN2 3
#define BUTTON_PIN3 4
#define BUTTON_PIN4 5

// Define file system for SD card
SdFs sd;
FsFile file;

// Configuration for FAT16/FAT32 and exFAT.
// Chip select may be constant or RAM variable.
const uint8_t SD_CS_PIN = A3;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

// SdFat software SPI template
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)

#define CHART_X 20           // X-coordinate of the leftmost point of the chart
#define CHART_Y_TEMP 40        //reference point
#define CHART_Y_HUMIDITY 40     //reerence point
#define CHART_Y_PRESSURE 160           // Y-coordinate of the bottom point of the chart as reference point
#define CHART_WIDTH 200      // Width of the chart area
#define CHART_DIFFERENCE 40   // Horizontal difference between datapoints
#define CHART_HEIGHT 100     // Height of the chart area
#define CHART_DATAPOINTS 5  // Number of datapoints to display on the chart
#define CHART_Y_LIGHT 5  // Adjust the Y-coordinate to leave some space at the top
#define CHART_HEIGHT_LIGHT 120 // Reduce the height to fit within the 128px height


// BME680 Setup
Adafruit_BME680 bme;

// BH1750 Setup
BH1750 lightMeter(0x23);

// SDI-12 Setup
#define DIRO 7 //Data Input Read Output; Arduino Due is set to receive data from SDI-12 based on the HIGH/LOW state

//initialize button
int button1;
int button2;
int button3;
int button4;

String message1;

//set up the string that will be used
String command;
String deviceAddress = "0";
String version = "14";
String unit = "ENG20009";
String ID1 = "103495";
String ID2 = "421";
String sensor = "4";
String deviceIdentification = "allccccccccmmmmmmvvvxxx";


float ArrayTemperature[5]; // Array to store 5 temperature readings
int ArrayTemperatureIndex = 0;        // Index to keep track of the current array position

float ArrayPressure[5]; // Array to store the last 5 pressure readings
int ArrayPressureIndex = 0; // Index to keep track of the current array position

float ArrayHumidity[5];
int ArrayHumidityIndex = 0;

float ArrayLight[5];
int ArrayLightIndex = 0;

int btnStatus = 0;
int Q = 0;
void setup() {
  Serial.begin(9600);

  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
  }

  if (!sd.begin(SD_CONFIG)) { // functionality of SD Card
    Serial.println("SD card initialization failed!");
    sd.initErrorHalt();
  }

  // Open/create a file for writing
  if (!file.open("message.txt", O_RDWR | O_CREAT)) { //Read/Write, Create
    sd.errorHalt(F("open failed"));
  }

  file.close(); // Release file

  // Set the temperature, pressure, and humidity oversampling
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);
  
  // BH1750 Initialization
  Wire.begin();
  lightMeter.begin();

  // SDI-12 Initialization
  Serial1.begin(1200, SERIAL_7E1); //baud rate 1200, data frame will have 7 bits; 1 stop bits
  pinMode(DIRO, OUTPUT);

  // HIGH to Receive from SDI-12
  digitalWrite(DIRO, HIGH);

  // make button pin mode input
  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN3, INPUT_PULLUP);
  pinMode(BUTTON_PIN4, INPUT_PULLUP);

  // assign interrupt to button, procedure to be run and set state to LOW when pushed
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN1),buttonPushed1,LOW);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN2),buttonPushed2,LOW);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN3),buttonPushed3,LOW);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN4),buttonPushed4,LOW);

  tft.initR(INITR_BLACKTAB);  // Init ST7735S chip, black tab
  tft.fillScreen(ST77XX_BLACK); // Initial make screen black
  tft.setRotation(3);  //rotate screen to landscape
}

void loop() {
  tft.fillScreen(ST77XX_BLACK); 
     
     int byte;
  //Receive SDI-12 over UART and then print to Serial Monitor
  while(Q != 1){
  if(Serial1.available()) {
    byte = Serial1.read();        //Reads incoming communication in bytes
    //Serial.println(byte);
    if (byte == 33) {             //If byte is command terminator (!)
      SDI12Receive(command);
      command = "";               //reset command string
    } else {
      if (byte != 0) {            //do not add start bit (0)
      command += char(byte);      //append byte to command string
      }
    }
  }
  }
  mainmenu(); // go mainmenu function

  readbutton(); // wait for readbutton

  if (button2 == 0) {
    bme.performReading();  
    float pressure = bme.pressure / 1000.0; // Divide by 100 to convert to hPa
    Serial.println("Pressure is " + String(pressure) ); 
    delay(20);
    if (ArrayPressureIndex < 5) {
      // Store the pressure reading in the array
      ArrayPressure[ArrayPressureIndex] = pressure;
      ArrayPressureIndex++;
    }

    displayPressureData();
    delay (2000);
    drawPressureChart();
    delay (2000);
    tft.setCursor(10, 110);  // set the coordinate of text
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE); // set the text colour
    tft.print("PB1-Back PB2-Save \n"); // print text to the screen
    readbutton(); //wait for readbutton
    if (button2 == 0){
    String pressure1 = String(bme.pressure); // get the measured data to string
    message1 += "The pressure: "; //combine the String
    message1 += pressure1; 
    message1 += "  "; 
    savedata(message1); //save the pressure to SDcard through this function
    }
    if(button1 == 0){
        return; //back to mainmenu
    }

  }

  if (button1 == 0) {
    bme.performReading();
    float temperature = bme.temperature;
    Serial.println("Temperature is " + String(temperature));
  if (temperature > 26){
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(1, 1);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_WHITE); 
      tft.print("temperature is higher than 26");
    Serial.println("temperature is higher than 26");
    delay(3000);
  }
  if (temperature < 26){
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(1, 1);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_WHITE); 
      Serial.println("temperature is less than 26");
  }
    if (ArrayTemperatureIndex < 5) {
      // Store the temperature reading in the array
      ArrayTemperature[ArrayTemperatureIndex] = temperature;
      ArrayTemperatureIndex++;
    }
    displayTemperatureReadings();
    delay(2000);
    drawTemperatureChart();
    delay (2000);
    tft.setCursor(10, 110); // set the coordinate of text
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE); // set the text colour
    tft.print("PB1-Back PB2-Save \n"); // print text to the screen
    readbutton(); //wait for readbutton
    if (button2 == 0){
    String temperature1 = String(bme.temperature); // get the measured data to string
    message1 += "The temperature: "; //combine the String
    message1 += temperature1;
    message1 += "  ";
    savedata(message1); //save the pressure to SDcard through this function
    }
    if(button1 == 0){
        return; //back to mainmenu
    }

  }

  if (button3 == 0) {
    // Add code to handle Button 3 (humidity) if needed

    bme.performReading();
    float humidity = bme.humidity; // Read humidity data
    Serial.println("Humidity is " + String(humidity) + "%");

    if (ArrayHumidityIndex < 5) {
      // Store the humidity reading in the array
      ArrayHumidity[ArrayHumidityIndex] = humidity;
      ArrayHumidityIndex++;
    }
    displayHumidityReadings();
    delay(2000);
    drawHumidityChart();
    delay (2000);
    tft.setCursor(10, 110); // set the coordinate of text
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE); // set the text colour
    tft.print("PB1-Back PB2-Save \n"); // print text to the screen
    readbutton(); //wait for readbutton
    if (button2 == 0){
    String humidity1 = String(bme.humidity); // get the measured data to string
    message1 += "The humidity: "; //combine the String
    message1 += humidity1;
    message1 += "  ";
    savedata(message1); //save the pressure to SDcard through this function
    }
    if(button1 == 0){
        return; //back to mainmenu
    }
  }

  if (button4 == 0) {
  float light = lightMeter.readLightLevel();
  Serial.println("Light level is " + String(light) + " lx");

  if (ArrayLightIndex < 5) {
    //Store the light reading in the array
    ArrayLight[ArrayLightIndex] = light;
    ArrayLightIndex++;
  }

  displayLightReadings();
  delay(2000);
  drawLightChart();
  delay (2000);
  tft.setCursor(10, 110); // set the coordinate of text
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE); // set the text colour
  tft.print("PB1-Back PB2-Save \n"); // print text to the screen
  readbutton(); //wait for readbutton
  if (button2 == 0){
  String light1 = String(lightMeter.readLightLevel()); // get the measured data to string
  message1 += "The light: "; //combine the String
  message1 += light1;
  message1 += "  ";
  savedata(message1); //save the pressure to SDcard through this function
  }
  if(button1 == 0){
      return; //back to mainmenu
  }
}
}

void readbutton(){
  Serial.println("readbutton");
  // initial set btnStatus = 0 and button = 1
  btnStatus = 0; 
  button1 = 1;
  button2 = 1;
  button3 = 1;
  button4 = 1;
  while (btnStatus !=1){ 
  if ((button1 == 0)||(button2 == 0)||(button3 == 0)|| (button4 == 0)){ //wait button be pressed
    if(button1 == 0){
    Serial.println("button1 pressed");
      // set other button = 1
      button2 = 1;
      button3 = 1;
      button4 = 1;
    }
    if(button2 == 0){
    Serial.println("button2 pressed");
     // set other button = 1
      button1 = 1;
      button3 = 1;
      button4 = 1;
    }
    if(button3 == 0){
    Serial.println("button3 pressed");
     // set other button = 1
      button2 = 1;
      button1 = 1;
      button4 = 1;
    }
    if(button4 == 0){
    Serial.println("button4 pressed");
     // set other button = 1
      button2 = 1;
      button3 = 1;
      button1 = 1;
    }
    btnStatus = 1; 
  }
    if(Serial1.available()){
      btnStatus = 1;
      Q = 0;
    }
  }

}

void mainmenu(){
  //display the menu
  tft.setCursor(1, 1);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Button1. Display Temp\n");
  tft.print("Button2. Pressure\n");
  tft.print("Button3. Humidity\n");
  tft.print("Button4. Light\n");
  Serial.println("mainmenu");
  delay(500);
}


//interrupt function
 void buttonPushed1(){
    button1 = 0;  // when button1 be pressed button1 = 0
 }

  void buttonPushed2(){
    button2 = 0; // when button2 be pressed button2 = 0
 }

  void buttonPushed3(){
    button3 = 0; // when button3 be pressed button3 = 0
 }

  void buttonPushed4(){
    button4 = 0; // when button4 be pressed button4 = 0
 }
 
 

void savedata(String message) {
  tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(1, 1);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_WHITE); 
      WriteSD(file, message);  // save data to the file in SDcard use this function
      ReadSD(file); // read data to the file in SDcard use this function
      tft.print("saved");
      delay(2000);
}

void WriteSD(FsFile file, String message) {
  Serial.println("---   Saving To File   ---");
  file.open("message.txt", O_RDWR); // opent the file in SDcard
  file.rewind(); //Go to file position 0
  file.println(message); //print the string to SDcard
  file.close(); // close the file
}

void ReadSD(FsFile file) {
  Serial.println("--- Reading From file! ---");
  file.open("message.txt", O_RDWR); // opent the file in SDcard
  file.seek(0); //go to char 0
  String contents = file.readString(); // get the string in file
  Serial.println(contents);
  file.close(); // close the file
}

void displayTemperatureReadings() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(1, 1);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Temperature Readings:");

  // Display the first 5 temperature readings
  for (int i = 0; i < 5; i++) {
    tft.print("Reading ");
    tft.print(i + 1);
    tft.print(": ");
    tft.println(ArrayTemperature[i]);
  }
}

void drawTemperatureChart() {
  tft.fillScreen(ST77XX_BLACK);

  // Draw chart outline
  tft.drawRect(CHART_X - 1, CHART_Y_TEMP - 1, CHART_WIDTH + 2, CHART_HEIGHT + 2, ST77XX_WHITE);

  // Draw horizontal grid lines
  for (int i = 1; i < 5; i++) {
    int y = CHART_Y_TEMP + i * (CHART_HEIGHT / 4);
    tft.drawFastHLine(CHART_X, y, CHART_WIDTH, ST77XX_WHITE);
  }

  // Draw vertical grid lines
  for (int i = 1; i < CHART_DATAPOINTS; i++) {
    int x = CHART_X + i * CHART_DIFFERENCE;
    tft.drawFastVLine(x, CHART_Y_TEMP, CHART_HEIGHT, ST77XX_WHITE);
  }

  // Draw the temperature data as a line chart
  for (int i = 0; i < CHART_DATAPOINTS; i++) {
    int x = CHART_X + i * CHART_DIFFERENCE;
    int y = CHART_Y_TEMP + (CHART_HEIGHT - ArrayTemperature[i] * 2);
    tft.fillCircle(x, y, 3, ST77XX_RED); // Draw a small red circle at each data point
    if (i > 0) {
      int x2 = CHART_X + (i - 1) * CHART_DIFFERENCE;
      int y2 = CHART_Y_TEMP + (CHART_HEIGHT - ArrayTemperature[i - 1] * 2);
      tft.drawLine(x, y, x2, y2, ST77XX_RED); // Draw a line connecting the data points
    }
  } 

  // Update the TFT display
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Temperature Chart");

  tft.setCursor(10, 180);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(10, 200);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
}

void displayPressureData() {
  tft.fillScreen(ST77XX_BLACK);

  // Display pressure data
  tft.setCursor(1, 1);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Pressure Data:");

  for (int i = 0; i < CHART_DATAPOINTS; i++) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GREEN); 
    tft.print("Data " + String(i + 1) + ": ");
    tft.println(ArrayPressure[i], 1); // Display pressure with one decimal place
    
  }
}

void drawPressureChart() {
  tft.fillScreen(ST77XX_BLACK);

  // Draw chart outline
  tft.drawRect(CHART_X - 1, CHART_Y_PRESSURE - 1, CHART_WIDTH + 2, CHART_HEIGHT + 2, ST77XX_WHITE);

  // Draw horizontal grid lines
  for (int i = 1; i < 5; i++) {
    int y = CHART_Y_PRESSURE + i * (CHART_HEIGHT / 4);
    tft.drawFastHLine(CHART_X, y, CHART_WIDTH, ST77XX_WHITE);
  }

  // Draw vertical grid lines
  for (int i = 1; i < CHART_DATAPOINTS; i++) {
    int x = CHART_X + i * CHART_DIFFERENCE;
    tft.drawFastVLine(x, CHART_Y_PRESSURE, CHART_HEIGHT, ST77XX_WHITE);
  }

  // Draw the pressure data as a line chart with data points
  for (int i = 0; i < CHART_DATAPOINTS; i++) {
    int x = CHART_X + i * CHART_DIFFERENCE;
    int y = CHART_Y_PRESSURE + (CHART_HEIGHT - ArrayPressure[i] * 2);
    tft.fillCircle(x, y, 3, ST77XX_GREEN); // Draw a small green circle at each data point
    if (i > 0) {
      int x2 = CHART_X + (i - 1) * CHART_DIFFERENCE;
      int y2 = CHART_Y_PRESSURE + (CHART_HEIGHT - ArrayPressure[i - 1] * 2);
      tft.drawLine(x, y, x2, y2, ST77XX_GREEN); // Draw a line connecting the data points
    }
  }

  // Update the TFT display
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Pressure Chart");

  tft.setCursor(10, 180);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(10, 200);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
}

void displayHumidityReadings() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(1, 1);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Humidity Readings:");

  // Display the humidity readings
  for (int i = 0; i < CHART_DATAPOINTS; i++) {
    tft.print("Reading ");
    tft.print(i + 1);
    tft.print(": ");
    tft.print(ArrayHumidity[i]);
    tft.println("%");
  }
}


void drawHumidityChart() {
  tft.fillScreen(ST77XX_BLACK);

  // Draw chart outline
  tft.drawRect(CHART_X - 1, CHART_Y_HUMIDITY - 1, CHART_WIDTH + 2, CHART_HEIGHT + 2, ST77XX_WHITE);

  // Draw horizontal grid lines for humidity chart
  for (int i = 1; i < 5; i++) {
    int y = CHART_Y_HUMIDITY + i * (CHART_HEIGHT / 4);
    tft.drawFastHLine(CHART_X, y, CHART_WIDTH, ST77XX_WHITE);
  }

  // Draw vertical grid lines for humidity chart
  for (int i = 1; i < CHART_DATAPOINTS; i++) {
    int x = CHART_X + i * CHART_DIFFERENCE;
    tft.drawFastVLine(x, CHART_Y_HUMIDITY, CHART_HEIGHT, ST77XX_WHITE);
  }

   // Draw the humidity data as a line chart with data points
  for (int i = 0; i < CHART_DATAPOINTS; i++) {
    int x = CHART_X + i * CHART_DIFFERENCE;
    int y = CHART_Y_HUMIDITY + (CHART_HEIGHT - ArrayHumidity[i] * 2);
    tft.fillCircle(x, y, 3, ST77XX_BLUE); // Draw a small blue circle at each data point
    if (i > 0) {
      int x2 = CHART_X + (i - 1) * CHART_DIFFERENCE;
      int y2 = CHART_Y_HUMIDITY + (CHART_HEIGHT - ArrayHumidity[i - 1] * 2);
      tft.drawLine(x, y, x2, y2, ST77XX_BLUE); // Draw a line connecting the data points
    }
  }

  // Update the TFT display
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Humidity Chart");

  tft.setCursor(10, 180);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(10, 200);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
}

void displayLightReadings() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(1, 1);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Light Sensor Readings:");

  for (int i = 0; i < 5; i++) {
    tft.print("Reading ");
    tft.print(i + 1);
    tft.print(": ");
    tft.print(ArrayLight[i]);
    tft.println(" lx");
  }
}

void drawLightChart() {
  tft.fillScreen(ST77XX_BLACK);

  // Adjust chart dimensions to fit the display
  int chartX = 5; // Adjust the X-coordinate to leave some space at the left
  int chartWidth = 150; // Adjust the width to fit within the 160px width


  // Draw chart outline
  tft.drawRect(CHART_X - 1, CHART_Y_LIGHT - 1, CHART_WIDTH + 2, CHART_HEIGHT_LIGHT + 2, ST77XX_WHITE);

  // Draw horizontal grid lines
  for (int i = 1; i < 5; i++) {
    int y = CHART_Y_LIGHT + i * (CHART_HEIGHT / 4);
    tft.drawFastHLine(CHART_X, y, CHART_WIDTH, ST77XX_WHITE);
  }

  // Draw vertical grid lines
  for (int i = 1; i < 5; i++) {
    int x = CHART_X + i * (CHART_WIDTH / 4);
    tft.drawFastVLine(x, CHART_Y_LIGHT, CHART_HEIGHT_LIGHT, ST77XX_WHITE);
  }

  // Draw the light data as a line chart with data points
  for (int i = 0; i < 5; i++) {
    int x = chartX + i * (chartWidth / 4);
    int y = CHART_Y_LIGHT + (CHART_HEIGHT_LIGHT - ArrayLight[i]);
    tft.fillCircle(x, y, 3, ST77XX_YELLOW); // Draw a small yellow circle at each data point
    if (i > 0) {
      int x2 = chartX + (i - 1) * (chartWidth / 4);
      int y2 = CHART_Y_LIGHT + (CHART_HEIGHT_LIGHT - ArrayLight[i - 1]);
      tft.drawLine(x, y, x2, y2, ST77XX_YELLOW); // Draw a line connecting the data points
    }
  }

  // Update the TFT display
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Light Sensor Chart");

  tft.setCursor(10, 180);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(10, 200);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
}

void SDI12Receive(String input) {
  //convert device address to string
  String address = String(deviceAddress);
  //Determines if the command is addressed for this device
  if (String(input.charAt(0)) == "?") {  
       SDI12Send(address);
  }
  
  if (
    (String(input.charAt(0)) == deviceAddress) && (String(input.charAt(1)) == "A") && (isDigit(input.charAt(2)))) {
       deviceAddress = input.substring(2,3);
       SDI12Send(deviceAddress);

  } 

  if((String(input.charAt(0)) == deviceAddress) && (String(input.charAt(1)) == "M")){
    long int starttime = millis(); //read time in millisecond
    bme.performReading();
    long int endtime = millis();
    //Get the time of measurement
    long int time = endtime - starttime; 
    SDI12Send3(address, time, sensor); //put these value to the function
    Serial.println("Responding to TEST command");
  }

    if((String(input.charAt(0)) == deviceAddress) && (String(input.charAt(1)) == "D")&& (String(input.charAt(2)) == "1")){
    String value;
    bme.performReading();
    //Get the data of measurement
    String humidity = String(bme.humidity);
    String temperature = String(bme.temperature);
    String gas = String(bme.gas_resistance);
    String pressure = String(bme.pressure);
    //combine the data as string
    value += address;
    value += "+";
    value += temperature;
    value += "+";
    value += pressure;
    value += "+";
    value += humidity;
    value += "+";
    value += gas;
    SDI12Send(value);
    Serial.println("Responding to TEST command");
  }

  if((String(input.charAt(0)) == deviceAddress) && (String(input.charAt(1)) == "I")){
  SDI12Send2(address, version, unit, ID1, ID2);
  Serial.println("Responding to send identification");

  }
    if((String(input.charAt(0)) == "Q")){
  Q = 1;

  }
}   



void SDI12Send(String message) {
  Serial.print("message: "); Serial.println(message);
  
  digitalWrite(DIRO, LOW);
  delay(100);
  Serial1.print(message + String("\r\n")); //print these string
  Serial1.flush();    //wait for print to finish
  Serial1.end();
  Serial1.begin(1200, SERIAL_7E1);
  digitalWrite(DIRO, HIGH);
}

void SDI12Send3(String address, int time, String sensor) {
  Serial.print("message: "); Serial.println(address);
  
  digitalWrite(DIRO, LOW);
  delay(100);
  Serial1.print(address + time + sensor + String("\r\n"));//print these string
  Serial1.flush();    //wait for print to finish
  Serial1.end();
  Serial1.begin(1200, SERIAL_7E1);
  digitalWrite(DIRO, HIGH);
}

void SDI12Send2(String address, String version, String unit, String ID1, String ID2) {
  Serial.print("message: "); Serial.println(address); //print address
  
  digitalWrite(DIRO, LOW);
  delay(100);
  Serial1.print(address + version + unit + ID1+ ID2 + String("\r\n")); //print these string
  Serial1.flush();    //wait for print to finish
  Serial1.end();
  Serial1.begin(1200, SERIAL_7E1);
  digitalWrite(DIRO, HIGH);
}
