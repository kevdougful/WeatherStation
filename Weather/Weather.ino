#include <MPL3115A2.h>
#include <HTU21D.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <Wire.h>
#include <stdlib.h>

// CC3000
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = 
  Adafruit_CC3000(
    ADAFRUIT_CC3000_CS, 
    ADAFRUIT_CC3000_IRQ, 
    ADAFRUIT_CC3000_VBAT,
    SPI_CLOCK_DIVIDER);
    
const unsigned long connectTimeout  = 15L * 1000L; // Max time to wait for server connection
uint32_t time;

Adafruit_CC3000_Client client;

#define WLAN_SSID       "COFFWILLHAUS"
#define WLAN_PASS       "butterball"
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Raspberry Pi base station
uint32_t pi;

// Weather shield
MPL3115A2 myPressure;
HTU21D myHumidity;
const byte REFERENCE_3V3 = A3;
const byte LIGHT = A1;
const byte BLUE = 7;
const byte GREEN = 8;

void setup()
{
  Serial.begin(115200);
  Serial.println("initiating connection...");
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(REFERENCE_3V3, INPUT);
  pinMode(LIGHT, INPUT);
  
  //Configure the pressure sensor
  myPressure.begin();              // Get sensor online
  myPressure.setModeBarometer();   // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags();   // Enable all three pressure and temp event flags 
  
  //Configure the humidity sensor
  myHumidity.begin();
  
  cc3000.begin();
  cc3000.deleteProfiles();
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) 
  {
    Serial.println("Failed!");
    while(1);
  } 
  else 
  {
    Serial.print("Connected to "); 
    Serial.println(WLAN_SSID);
    digitalWrite(GREEN, HIGH);
  } 
  Serial.println("Requesting DHCP");
  while (!cc3000.checkDHCP())
  {
    digitalWrite(BLUE, HIGH);
    delay(200); // ToDo: Insert a DHCP timeout!
    digitalWrite(BLUE, LOW);
  }
  Serial.println("DHCP ok");
  
  pi = cc3000.IP2U32(192,168,1,98);
}

void loop()
{
  digitalWrite(BLUE, HIGH);
  char buf[10];
  String t, h, p, l;
  time = millis();
  do 
  {
      client = cc3000.connectTCP(pi, 80);
  } 
  while((!client.connected()) && ((millis() - time) < connectTimeout));
  if (client.connected())
  {
    Serial.println("Connected to pi");
    t = dtostrf(myPressure.readTempF(), 5, 2, buf);
    h = dtostrf(myHumidity.readHumidity(), 5, 2, buf);
    p = dtostrf(myPressure.readPressure() / 1000, 4, 2, buf);
    l = dtostrf(getLight(), 1, 2, buf);
    String request = "GET /upload.php?temp=" + t
                     + "&humd=" + h
                     + "&pres=" + p
                     + "&lite=" + l;
    Serial.println(request);
    sendRequest(request);
  }
  digitalWrite(BLUE, LOW);
  delay(15 * 1000);
}
bool sendRequest (String request) {
  // Transform to char
  char requestBuf[request.length()+1];
  request.toCharArray(requestBuf,request.length()); 
  // Send request
  if (client.connected()) {
    client.fastrprintln(requestBuf); 
  } 
  else {
    Serial.println(F("Connection failed"));    
    return false;
  }
  return true;
  free(requestBuf);
}

float getLight()
{
  float voltage = analogRead(REFERENCE_3V3);
  float sensor = analogRead(LIGHT);
  voltage = 3.3 / voltage;
  return sensor * voltage;  
}
