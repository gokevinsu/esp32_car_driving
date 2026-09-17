#include <SPI.h>
#include <RF24.h>

unsigned long lastPacketTime = 0;
const int timeout = 200;
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
enum mode {driveRadio, Auto};
mode currentMode = driveRadio;

//pins 
const uint8_t frpwm = ;
const uint8_t frm1 = ;
const uint8_t frm2 = ;

const uint8_t flpwm = ;
const uint8_t flm1 = ;
const uint8_t flm2 = ;

const uint8_t brpwm = ;
const uint8_t brm1 = ;
const uint8_t brm2 = ;

const uint8_t blpwm = ;
const uint8_t blm1 = ;
const uint8_t blm2 = ;

const uint8_t fre1 = ;
const uint8_t fre2 = ;

const uint8_t fle1 = ;
const uint8_t fle2 = ;

const uint8_t bre1 = ;
const uint8_t bre2 = ;

const uint8_t ble1 = ;
const uint8_t ble2 = ;

const uint8_t miso = ;
const uint8_t mosi = ;
const uint8_t sck = ;
const uint8_t csn = ;
const uint8_t ce = ;

const uint8_t sda = ;
const uint8_t scl = ;

const uint8_t stby = ;

const uint8_t batteryPinSense = ;

const uint8_t batterypinLed = ;

const uint64_t radioAddress = 0xF0F0F0F0E1LL;

struct ControlPacket {
  int16_t x;
  int16_t y;
  int16_t z;
};

ControlPacket packet;

RF24 radio(ce, csn);

void setup() {
  Serial.begin(115200);

  SPI.begin();
  if (!radio.begin()) {
    Serial.println("RF24 radio not detected");
    Serial.print("connecting");
    int count = 0;
    while(!radio.begin()) {
      delay(500);
      count++;
      Serial.print(".");
      if(count > 10) {
        while(1);
      }
    }
  }
  radio.setChannel(108);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, radioAddress);
  radio.startListening();
  Serial.println("RF24 receiver ready");

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

  pinMode(batteryPinSense, INPUT);
  ledcAttach(batterypinLed, 2000, 12);
  pinMode(stby, OUTPUT);
}


void loop() {
  v = analogRead(batteryPinSense) * (3.3 / 4095.0) * dividerRatio;
  percent = 0;
  for (int step = 0; step <= 20; step++) {
    if (v >= voltageByPercent[step]) {
      percent = step * 5;
    }
  }
  ledcWrite(batterypinLed, map(percent, 0, 100, 0, 4095));
  digitalWrite(stby, standby);

  //radio
  if(currentMode == driveRadio){
    while (radio.available()) {
      radio.read(&packet, sizeof(packet));
      lastPacketTime = millis();
      fl = constrain(packet.x + packet.y + packet.z, -4095, 4095);
      fr = constrain(packet.x - packet.y - packet.z, -4095, 4095);
      bl = constrain(packet.x - packet.y + packet.z, -4095, 4095);
      br = constrain(packet.x + packet.y - packet.z, -4095, 4095);
    }
  }

  //auto
  if(currentMode == Auto){
    
  }
  
  if (millis() - lastPacketTime > timeout) {
    standby = LOW;
  }
  else{
    standby = HIGH;
  }
}