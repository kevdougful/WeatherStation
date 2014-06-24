#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <Time.h>
#include <OneWire.h>
#include <SD.h>

byte macAddr[] = 
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD };  // Ethernet shield MAC address
unsigned int UdpPort = 8888;               // UDP port to receive time data from NIST
EthernetUDP Udp;                           // Arduino Udp object
IPAddress timeServer(216, 171, 120, 36);   // nist1-ch1.ustiming.org (Chicago)
const int NTP_PACKET_SIZE = 48;            // NTP time stamp is found in first 48 bytes
byte packetBuffer[NTP_PACKET_SIZE];        // Buffer to hold incoming and outgoing packets               
const int DELAY_SECONDS = 60;
const int DS18B20_PIN = 7;
const int SD_CHIP_SELECT = 8;
const int ETH_CHIP_SELECT = 10;
OneWire ds(DS18B20_PIN);
File logFile;
time_t t;

void setup()
{
    Serial.begin(9600); 
    Serial.println("starting Ethernet");
    Eth_On();
    if (Ethernet.begin(macAddr) == 0)
    {
        Serial.println("DHCP failure");
        while (true);  // No connection, so halt
    }
    delay(1000);
    if(!Udp.begin(UdpPort)) Serial.println("Udp error");
    else Serial.println("Udp started");
    t = GetNtpTime();
    SD_On();
    if (!SD.begin(SD_CHIP_SELECT))
    {
        Serial.println("SD init failed.");
        return;
    }
    Serial.println("SD init successful.");
}

void loop()
{
    time_t tm = t + (millis() / 1000);
    Serial.print(tm);
    Serial.print(",");
    Serial.print(GetTemp());
    Serial.println();
    logFile = SD.open("log.txt", FILE_WRITE);
    logFile.print(tm);
    logFile.print(",");
    logFile.print(GetTemp());
    logFile.println();
    logFile.close();
    delay(10000);
}

// Builds and sends a request packet and stores the response in packetBuffer
unsigned long SendNtpPacket(IPAddress& addr)
{
    // Clear out packetBuffer
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Build request packet...
    packetBuffer[0] = 0b11100011;   // Leap Indicator, Version, Mode
    packetBuffer[1] = 0;            // Stratum, or type of clock
    packetBuffer[2] = 6;            // Polling Interval
    packetBuffer[3] = 0xEC;         // Peer Clock Precision
                                    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;         // Reference identifier (32-bits)...
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    
    Udp.beginPacket(addr, 123);  // Port 123 listens for NTP requests
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
    delay(1000);  // wait for response
    Udp.parsePacket();
    Udp.read(packetBuffer, NTP_PACKET_SIZE);
}

// Parses an NTP response packet and returns the seconds passed since 1/1/1970 
unsigned long SecsSince1970(byte buffer[])
{
    unsigned long highWord = word(buffer[40], buffer[41]);
    unsigned long lowWord = word(buffer[42], buffer[43]);
    return (highWord << 16 | lowWord) - 2208988800;
}

unsigned long GetNtpTime()
{
   SendNtpPacket(timeServer);
   return SecsSince1970(packetBuffer);
}

float GetTemp()
{
    byte data[12];
    byte addr[8];
    
    if (!ds.search(addr))
    {
       ds.reset_search();
       return -1000; 
    }
  
    if (OneWire::crc8(addr, 7) != addr[7])
    {
       Serial.println("CRC is not valid!");
       return -1000; 
    }
    
    if (addr[0] != 0x10 && addr[0] != 0x28)
    {
       Serial.print("Device is not recognized");
       return -1000;
    }
    
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);  // Start communication with the sensor.
    
    byte present = ds.reset();
    ds.select(addr);
    ds.write(0xBE); // Read Scratchpad.
    
    for (int i = 0; i < 9; i++)  // need 9 bytes
    {
        data[i] = ds.read(); 
    }
    
    ds.reset_search();
    
    byte MSB = data[1];  // most significant byte
    byte LSB = data[0];  // lease significant byte
    
    float tempRead = ((MSB << 8) | LSB);  
    float TemperatureSum = tempRead / 16;
    
    return TemperatureSum;
  
}
void SD_On()
{
    pinMode(SD_CHIP_SELECT, OUTPUT);
    pinMode(ETH_CHIP_SELECT, OUTPUT);
    digitalWrite(ETH_CHIP_SELECT, HIGH);
    digitalWrite(SD_CHIP_SELECT, LOW); 
}
void Eth_On()
{
    pinMode(SD_CHIP_SELECT, OUTPUT);
    pinMode(ETH_CHIP_SELECT, OUTPUT);
    digitalWrite(ETH_CHIP_SELECT, LOW);
    digitalWrite(SD_CHIP_SELECT, HIGH); 
}

