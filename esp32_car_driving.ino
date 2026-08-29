#include <WiFi.h>
#include <WiFiUdp.h>
//#include <SPI.h>
//#include <RF24.h>
#include "USB.h"

HWCDC USBOtgSerial; 
WiFiUDP udp;
//RF24 radio(9, 10);

const uint16_t port = 4210;
const uint32_t discoveryMagic = 0x43415231;
struct __attribute__((packed)) WifiJoystickPacket{
  int16_t x;
  int16_t y;
  int16_t z;
};
WifiJoystickPacket packet;

unsigned long lastPacketTime = 0;
const int timeout = 200;
const int motorThreshold = 1230;
bool standby = HIGH;
int percent = 0;
float v = 0;
int fr;
int fl;
int br;
int bl;
const float dividerRatio = (1000000.0 + 4700.0) / 4700.0;
const float voltageByPercent[] = {
  9.00, 9.90, 10.50, 10.80, 11.00, 11.10, 11.20, 11.30, 11.40,
  11.50, 11.60, 11.70, 11.80, 11.90, 12.00, 12.10, 12.20, 12.30,
  12.40, 12.50, 12.60
};
unsigned long lastDebugTime = 0;
const int debugInterval = 50;
enum mode {driveWifi, driveRadio, Auto};
mode currentMode = driveWifi;
//const byte radioAddress[6] = "CAR01";

//pins 
const uint8_t frpwm = 17;
const uint8_t frm1 = 15;
const uint8_t frm2 = 16;

const uint8_t flpwm = 47;
const uint8_t flm1 = 48;
const uint8_t flm2 = 45;

const uint8_t brpwm = 4;
const uint8_t brm1 = 5;
const uint8_t brm2 = 6;

const uint8_t blpwm = 1;
const uint8_t blm1 = 0;
const uint8_t blm2 = 2;

const uint8_t fre1 = 37;
const uint8_t fre2 = 38;

const uint8_t fle1 = 35;
const uint8_t fle2 = 36;

const uint8_t bre1 = 40;
const uint8_t bre2 = 39;

const uint8_t ble1 = 41;
const uint8_t ble2 = 42;

const uint8_t statusLedPin = 3;

const uint8_t stby = 7;

const uint8_t batteryPin = 8;

void setup() {
  Serial.begin(115200);
  USBOtgSerial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  udp.begin(port);

  /*SPI.begin();
  if (!radio.begin()) {
    Serial.println("RF24 radio not detected");
  }
  else {
    radio.setChannel(108);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_LOW);
    radio.openReadingPipe(1, radioAddress);
    radio.startListening();
    Serial.println("RF24 receiver ready");
  }*/

  delay(1000);
  ledcAttach(frpwm, 2000, 12);
  ledcAttach(brpwm, 2000, 12);
  ledcAttach(flpwm, 2000, 12);
  ledcAttach(blpwm, 2000, 12);

  pinMode(frm1, OUTPUT);
  pinMode(frm2, OUTPUT);

  pinMode(blm1, OUTPUT);
  pinMode(blm2, OUTPUT);

  pinMode(flm1, OUTPUT);
  pinMode(flm2, OUTPUT);

  pinMode(brm1, OUTPUT);
  pinMode(brm2, OUTPUT);

  pinMode(fle1, INPUT);
  pinMode(fle2, INPUT);

  pinMode(bre1, INPUT);
  pinMode(bre2, INPUT);

  pinMode(ble1, INPUT);
  pinMode(ble2, INPUT);

  pinMode(fre1, INPUT);
  pinMode(fre2, INPUT);

  pinMode(batteryPin, INPUT);
  pinMode(statusLedPin, OUTPUT);
  pinMode(stby, OUTPUT);
}


void loop() {
  v = analogRead(batteryPin) * (3.3 / 4095.0) * dividerRatio;
  percent = 0;
  for (int step = 0; step <= 20; step++) {
    if (v >= voltageByPercent[step]) {
      percent = step * 5;
    }
  }
  digitalWrite(statusLedPin, (WiFi.softAPgetStationNum() > 0 ? HIGH:LOW));
  digitalWrite(stby, standby);

  //wifi
  if(currentMode == driveWifi){
    int packetSize;
    while ((packetSize = udp.parsePacket()) > 0) {
      if (packetSize == sizeof(discoveryMagic)) {
        uint32_t receivedMagic;
        udp.read((uint8_t*)&receivedMagic, sizeof(receivedMagic));
        if (receivedMagic == discoveryMagic) {
          udp.beginPacket(udp.remoteIP(), udp.remotePort());
          udp.write((uint8_t*)&discoveryMagic, sizeof(discoveryMagic));
          udp.endPacket();
        }
      }
      else if (packetSize == sizeof(WifiJoystickPacket) && udp.read((uint8_t*)&packet, sizeof(packet)) == sizeof(WifiJoystickPacket)) {
        lastPacketTime = millis();
        fl = constrain(packet.x + packet.y + packet.z, -4095, 4095);
        fr = constrain(packet.x - packet.y - packet.z, -4095, 4095);
        bl = constrain(packet.x - packet.y + packet.z, -4095, 4095);
        br = constrain(packet.x + packet.y - packet.z, -4095, 4095);
      }
    }
  }

  //radio
  /*if(currentMode == driveRadio){
    while (radio.available()) {
      radio.read(&packet, sizeof(packet));
      lastPacketTime = millis();
      fl = constrain(packet.x + packet.y + packet.z, -4095, 4095);
      fr = constrain(packet.x - packet.y - packet.z, -4095, 4095);
      bl = constrain(packet.x - packet.y + packet.z, -4095, 4095);
      br = constrain(packet.x + packet.y - packet.z, -4095, 4095);
    }
  }*/

  //auto
  if(currentMode == Auto){
    if (USBOtgSerial.available() >= 10) {
      if (USBOtgSerial.read() == 0xAA) {
        if (USBOtgSerial.read() == 0xBB) {
          uint8_t dataBuffer[8];
          USBOtgSerial.readBytes(dataBuffer, 8);
          fr = constrain((int16_t)(dataBuffer[0] | (dataBuffer[1] << 8)), -4095, 4095);
          fl = constrain((int16_t)(dataBuffer[2] | (dataBuffer[3] << 8)), -4095, 4095);
          br = constrain((int16_t)(dataBuffer[4] | (dataBuffer[5] << 8)), -4095, 4095);
          bl = constrain((int16_t)(dataBuffer[6] | (dataBuffer[7] << 8)), -4095, 4095);
          lastPacketTime = millis();
        }
      }
    }
  }
  
  if (millis() - lastPacketTime > timeout) {
    standby = LOW;
  }
  else{
    standby = HIGH;
  }

  if (millis() - lastDebugTime >= debugInterval) {
    Serial.print("M FR:");
    Serial.print(fr);
    Serial.print(" FL:");
    Serial.print(fl);
    Serial.print(" BR:");
    Serial.print(br);
    Serial.print(" BL:");
    Serial.print(bl);
    Serial.print(" STBY:");
    Serial.print(standby);
    Serial.print(" BAT:");
    Serial.print(percent);
    Serial.println("%");
    lastDebugTime = millis();
  }
}