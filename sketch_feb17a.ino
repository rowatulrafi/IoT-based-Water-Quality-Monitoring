#include <Arduino.h>  //basic function for arduino
#include <Wire.h> //allows communication with I2C devices; I2C has two lines SCL and SDA, SCL is used for clock and SDA is used for data.
#include <EEPROM.h> //provides functions to read and write to the EEPROM (Electrically Erasable Programmable Read-Only Memory) 
#include <WiFi.h> //enables Wi-Fi connectivity 
#include <OneWire.h>
#include <DallasTemperature.h> // typically used to interface with OneWire temperature sensors, such as the DS18B20
#include <Adafruit_ADS1X15.h>  //supports the ADS1015 and ADS1115 analog-to-digital converters (ADCs) from Adafruit.
#include <DFRobot_ESP_EC.h> // used for interfacing with electrical conductivity (EC) sensors
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> //support OLED displays, such as the SSD1306, and provide graphics functions and text on the display.
 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
 
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
 
#define ONE_WIRE_BUS 4                // this is the gpio pin 4 on esp32.
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
 
DFRobot_ESP_EC ec;
Adafruit_ADS1115 ads;
 
float voltage, ecValue, pH, Value=0, temperature = 25;
int buf[10],temp;
 
String apiKey = "QLH883C5C51W5UIJ";     // API key from ThingSpeak
 
const char *ssid =  "TP-Link_E754";     // wifi ssid and password
const char *pass =  "78222908";
const char* server = "api.thingspeak.com";
const int potpin = A3;
 
WiFiClient client;
 
 
void setup()
{
  Serial.begin(115200);   //Begins serial communication at a baud rate of 115200 for debugging.
  EEPROM.begin(32);//nitializes the EEPROM to store calibration data.
  pinMode(potpin,INPUT);
  ec.begin();
  sensors.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //specifies the I2C address of the display
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);   //infinite loop
  }
  delay(2000);
  display.clearDisplay();
 
  Serial.println("Connecting to ");
  Serial.println(ssid);
 
 
  WiFi.begin(ssid, pass);
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}
 void loop()
{
  voltage = analogRead(A0); // VP in esp32 is the gpio 36
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);  // read temperature sensor to execute temperature compensation
  ecValue = ec.readEC(voltage, temperature); // convert voltage to EC with temperature compensation
 
 for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(potpin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }

  Value=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    Value+=buf[i];
  float phValue=(float)Value*3.3/4095/6; //convert the analog into millivolt
  phValue=3.3*phValue;


  

  Serial.print("Temperature:");
  Serial.print(temperature, 2);
  Serial.println("ºC");
 
  Serial.print("EC:");
  Serial.println(ecValue, 2);
 
  Serial.print("pH:");
  Serial.println(phValue, 2);

  display.setTextSize(2);
  display.setTextColor(WHITE);
 
  display.setCursor(0, 0);
  display.print("T:");
  display.print(temperature, 2);
  display.drawCircle(85, 0, 2, WHITE); // put degree symbol ( ° )
  display.setCursor(90, 0);
  display.print("C");
 
  display.setCursor(0, 20);
  display.print("EC:");
  display.print(ecValue, 2);

  display.setCursor(0, 40);
  display.print("pH:");
  display.print(phValue, 2);
  display.display();
  delay(500);
  display.clearDisplay();
 
 
  ec.calibration(voltage, temperature); // calibration process
 
  if (client.connect(server, 80))  //   api.thingspeak.com
  {
 
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temperature, 2);
    postStr += "&field2=";
    postStr += String(ecValue, 2);
    postStr += "&field3=";
    postStr += String(phValue, 2);
    postStr += "\r\n\r\n\r\n";
    delay(500);
 
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n\n");
    client.print(postStr);
    delay(500);
  }
  client.stop();
}
