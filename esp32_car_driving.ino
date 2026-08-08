#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "DEA";
const char* password = "40542992";

WiFiUDP udp;

const uint16_t port = 4210;

struct JoystickPacket {
  int16_t x;
  int16_t y;
  int16_t z;
};
JoystickPacket packet;


unsigned long lastPacketTime = 0;
const unsigned long timeout = 200;


// Return X axis
int a() {
  return packet.x;
}


// Return Y axis
int b() {
  return packet.y;
}


// Return Z axis
int c() {
  return packet.z;
}


// =========================
// Motor pins
// =========================

const uint8_t frpwm = 22;
const uint8_t fr1 = 20;
const uint8_t fr2 = 21;

const uint8_t brpwm = 23;
const uint8_t br1 = 25;
const uint8_t br2 = 24;

const uint8_t flpwm = 28;
const uint8_t fl1 = 27;
const uint8_t fl2 = 26;

const uint8_t blpwm = 30;
const uint8_t bl1 = 29;
const uint8_t bl2 = 31;

const int deadZone = 20;
const int motorThreshold = 4095 * 3 / 10;


// =========================
// Setup
// =========================

void setup() {

  Serial.begin(115200);

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected!");

  Serial.print("P4 IP: ");
  Serial.println(WiFi.localIP());

  // UDP
  udp.begin(port);

  Serial.print("Listening on port ");
  Serial.println(port);


  // PWM
  ledcAttach(frpwm, 2000, 12);
  ledcAttach(brpwm, 2000, 12);
  ledcAttach(flpwm, 2000, 12);
  ledcAttach(blpwm, 2000, 12);


  // Direction pins
  pinMode(fl1, OUTPUT);
  pinMode(fl2, OUTPUT);

  pinMode(bl1, OUTPUT);
  pinMode(bl2, OUTPUT);

  pinMode(fr1, OUTPUT);
  pinMode(fr2, OUTPUT);

  pinMode(br1, OUTPUT);
  pinMode(br2, OUTPUT);


  // Start centered
  packet.x = 2048;
  packet.y = 2048;
  packet.z = 2048;
}


// =========================
// Loop
// =========================

void loop() {

  // =========================
  // Receive joystick packet
  // =========================

  int packetSize = udp.parsePacket();

  if (packetSize == sizeof(JoystickPacket)) {

    int bytesRead = udp.read(
      (uint8_t*)&packet,
      sizeof(packet));

    if (bytesRead == sizeof(JoystickPacket)) {

      // Valid packet received
      lastPacketTime = millis();
    }
  }


  // =========================
  // Communication timeout
  // =========================

  if (millis() - lastPacketTime > timeout) {

    // Immediately stop all motors
    setMotor(0, fl1, fl2, flpwm);
    setMotor(0, fr1, fr2, frpwm);
    setMotor(0, bl1, bl2, blpwm);
    setMotor(0, br1, br2, brpwm);

    return;
  }


  // =========================
  // Get joystick values
  // =========================

  int throttle = map(
    b(),
    0,
    4095,
    -4095,
    4095);

  int strafe = map(
    a(),
    0,
    4095,
    -4095,
    4095);

  int rotate = map(
    c(),
    0,
    4095,
    -4095,
    4095);


  // =========================
  // Dead zone
  // =========================

  if (abs(throttle) < deadZone) {
    throttle = 0;
  }

  if (abs(strafe) < deadZone) {
    strafe = 0;
  }

  if (abs(rotate) < deadZone) {
    rotate = 0;
  }


  // =========================
  // Mecanum motor calculations
  // =========================

  int frontLeftSpeed =
    throttle + strafe + rotate;

  int frontRightSpeed =
    throttle - strafe - rotate;

  int rearLeftSpeed =
    throttle - strafe + rotate;

  int rearRightSpeed =
    throttle + strafe - rotate;
  // =========================
  // Limit motor speeds
  // =========================

  frontLeftSpeed =
    constrain(frontLeftSpeed, -4095, 4095);

  frontRightSpeed =
    constrain(frontRightSpeed, -4095, 4095);

  rearLeftSpeed =
    constrain(rearLeftSpeed, -4095, 4095);

  rearRightSpeed =
    constrain(rearRightSpeed, -4095, 4095);


  // =========================
  // Drive motors
  // =========================

  setMotor(
    frontLeftSpeed,
    fl1,
    fl2,
    flpwm);

  setMotor(
    frontRightSpeed,
    fr1,
    fr2,
    frpwm);

  setMotor(
    rearLeftSpeed,
    bl1,
    bl2,
    blpwm);

  setMotor(
    rearRightSpeed,
    br1,
    br2,
    brpwm);
}



void setMotor(int speed, uint8_t in1, uint8_t in2, uint8_t pwmPin) {

  speed = constrain(speed, -4095, 4095);

  if (abs(speed) > motorThreshold) {

    if (speed > 0) {

      // Forward
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      ledcWrite(pwmPin, abs(speed));

    } else {

      // Reverse
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      ledcWrite(pwmPin, abs(speed));
    }

  } else {

    // Stop
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    ledcWrite(pwmPin, 0);
  }
}