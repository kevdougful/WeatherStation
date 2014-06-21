#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <Time.h>
#include <OneWire.h>
#include <SD.h>

byte macAddr[] = 
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Ethernet shield MAC address
unsigned int UdpPort = 8888;               // UDP port to receive time data from NIST
EthernetUDP Udp;                           // Arduino Udp object
IPAddress timeServer(216, 171, 120, 36);   // nist1-ch1.ustiming.org (Chicago)
const int NTP_PACKET_SIZE = 48;            // NTP time stamp is found in first 48 bytes
byte packetBuffer[NTP_PACKET_SIZE];        // Buffer to hold incoming and outgoing packets               
const long TZ_OFFSET = -18000;
const int DELAY_SECONDS = 60;
const int DS18B20_PIN = 7;
OneWire ds(DS18B20_PIN);
File logFile;
const int SD_CHIP_SELECT = 8;

void setup()
{
    if (Ethernet.begin(macAddr) == 0)
    {
        Serial.println("DHCP failure");
        while (true);  // No connection, so halt
    }
    Udp.begin(UdpPort);
    setSyncProvider(GetNtpTime);
    while(timeStatus() == timeNotSet);  // wait for sync
    pinMode(10, OUTPUT);
    Serial.begin(9600); 
    if (!SD.begin(SD_CHIP_SELECT))
    {
        Serial.println("SD init failed.");
        return;
    }
    Serial.println("SD init successful.");
    logFile = SD.open("log.txt", FILE_WRITE);
    if (logFile)
        logFile.println("Time,Temp (°C)");
    logFile.close();
}

void loop()
{
    String d = ISODateTime();
    Serial.print(d);
    Serial.print(",");
    Serial.print(GetTemp());
    Serial.println();
    logFile = SD.open("log.txt", FILE_WRITE);
    logFile.print(d);
    logFile.print(",");
    logFile.print(GetTemp());
    logFile.println();
    delay(DELAY_SECONDS * 1000);
    logFile.close();
}

// Builds and sends a request packet and stores the response in packetBuffer
unsigned long SendNtpPacket(IPAddress addr)
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
    return (highWord << 16 | lowWord) - 2208988800 + TZ_OFFSET;
}

unsigned long GetNtpTime()
{
   SendNtpPacket(timeServer);
   return SecsSince1970(packetBuffer);
}

// Pads with zero if necessary
String pad(int num)
{
    if (num < 10 && num > 0) return "0" + String(num);
    else return String(num);
}

String TZD()
{
    String str;
    if (TZ_OFFSET > 0)
        str = "+";
    return str + pad(TZ_OFFSET / 3600) + ":00";
}

String ISODateTime()
{
    return String(year()) + "-" + pad(month()) + "-" + pad(day())        // Date
        + "T" + pad(hour()) + ":" + pad(minute()) + ":" + pad(second())  // Time
        + "+05:00";                                                      // Time zone
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
    
    byte MSB = data[1];  // most significant bit
    byte LSB = data[0];  // lease significant bit
    
    float tempRead = ((MSB << 8) | LSB);  
    float TemperatureSum = tempRead / 16;
    
    return TemperatureSum;
  
}


