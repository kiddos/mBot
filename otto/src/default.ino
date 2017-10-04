#include <MeMCore.h>
#include <Wire.h>

Servo servos[8];
MeUltrasonicSensor us;
MePort generalDevice;
MeBuzzer buzzer;

typedef struct MeModule {
  int device;
  int port;
  int slot;
  int pin;
  int index;
  float values[3];
} MeModule;

union {
  byte byteVal[4];
  float floatVal;
  long longVal;
} val;

union {
  byte byteVal[8];
  double doubleVal;
} valDouble;

union {
  byte byteVal[2];
  short shortVal;
} valShort;

#if defined(__AVR_ATmega32U4__)
const int analogs[12] PROGMEM = {A0, A1, A2, A3, A4,  A5,
                                 A6, A7, A8, A9, A10, A11};
#else
const int analogs[8] PROGMEM = {A0, A1, A2, A3, A4, A5, A6, A7};
#endif

String mVersion = "06.01.108";
boolean isAvailable = false;

int len = 52;
char buffer[52];
byte index = 0;
byte dataLen;
byte modulesLen = 0;
boolean isStart = false;
char serialRead;
uint8_t command_index = 0;
String irBuffer = "";
double lastTime = 0.0;
double currentTime = 0.0;
double lastIRTime = 0.0;

#define VERSION 0
#define ULTRASONIC_SENSOR 1
#define TEMPERATURE_SENSOR 2
#define LIGHT_SENSOR 3
#define POTENTIONMETER 4
#define JOYSTICK 5
#define GYRO 6
#define SOUND_SENSOR 7
#define RGBLED 8
#define SEVSEG 9
#define MOTOR 10
#define SERVO 11
#define ENCODER 12
#define IR 13
#define IRREMOTE 14
#define PIRMOTION 15
#define INFRARED 16
#define LINEFOLLOWER 17
#define IRREMOTECODE 18
#define SHUTTER 20
#define LIMITSWITCH 21
#define BUTTON 22
#define HUMITURE 23
#define FLAMESENSOR 24
#define GASSENSOR 25
#define COMPASS 26
#define TEMPERATURE_SENSOR_1 27
#define DIGITAL 30
#define ANALOG 31
#define PWM 32
#define SERVO_PIN 33
#define TONE 34
#define BUTTON_INNER 35
#define ULTRASONIC_ARDUINO 36
#define PULSEIN 37
#define STEPPER 40
#define LEDMATRIX 41
#define TIMER 50
#define TOUCH_SENSOR 51
#define JOYSTICK_MOVE 52
#define COMMON_COMMONCMD 60
// Secondary command
#define SET_STARTER_MODE 0x10
#define SET_AURIGA_MODE 0x11
#define SET_MEGAPI_MODE 0x12
#define GET_BATTERY_POWER 0x70
#define GET_AURIGA_MODE 0x71
#define GET_MEGAPI_MODE 0x72
#define ENCODER_BOARD 61
// Read type
#define ENCODER_BOARD_POS 0x01
#define ENCODER_BOARD_SPEED 0x02

#define ENCODER_PID_MOTION 62
// Secondary command
#define ENCODER_BOARD_POS_MOTION 0x01
#define ENCODER_BOARD_SPEED_MOTION 0x02
#define ENCODER_BOARD_PWM_MOTION 0x03
#define ENCODER_BOARD_SET_CUR_POS_ZERO 0x04
#define ENCODER_BOARD_CAR_POS_MOTION 0x05

#define PM25SENSOR 63
// Secondary command
#define GET_PM1_0 0x01
#define GET_PM2_5 0x02
#define GET_PM10 0x03

#define GET 1
#define RUN 2
#define RESET 4
#define START 5
float angleServo = 90.0;
int servo_pins[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char prevc = 0;
boolean buttonPressed = false;
uint8_t keyPressed = 0;

/*
 * function list
 */
void buzzerOn();
void buzzerOff();
unsigned char readBuffer(int index);
void writeBuffer(int index, unsigned char c);
void writeHead();
void writeEnd();
void writeSerial(unsigned char c);
void readSerial();
void parseData();
void callOK();
void sendByte(char c);
void sendString(String s);
void sendFloat(float value);
void sendShort(double value);
void sendDouble(double value);
short readShort(int idx);
float readFloat(int idx);
char* readString(int idx, int len);
uint8_t* readUint8(int idx, int len);
void runModule(int device);
int searchServoPin(int pin);
void readSensor(int device);

void buzzerOn() { buzzer.tone(500, 1000); }

void buzzerOff() { buzzer.noTone(); }

unsigned char readBuffer(int index) { return buffer[index]; }

void writeBuffer(int index, unsigned char c) { buffer[index] = c; }

void writeHead() {
  writeSerial(0xff);
  writeSerial(0x55);
}

void writeEnd() { Serial.println(); }

void writeSerial(unsigned char c) { Serial.write(c); }

void readSerial() {
  isAvailable = false;
  if (Serial.available() > 0) {
    isAvailable = true;
    serialRead = Serial.read();
  }
}

/*
ff 55 len idx action device port slot data a
0  1  2   3   4      5      6    7    8
*/
void parseData() {
  isStart = false;
  int idx = readBuffer(3);
  command_index = (uint8_t)idx;
  int action = readBuffer(4);
  int device = readBuffer(5);
  switch (action) {
    case GET: {
      if (device != ULTRASONIC_SENSOR) {
        writeHead();
        writeSerial(idx);
      }
      readSensor(device);
      writeEnd();
    } break;
    case RUN: {
      runModule(device);
      callOK();
    } break;
    case RESET: {
      // reset
      buzzerOff();
      callOK();
    } break;
    case START: {
      // start
      callOK();
    } break;
  }
}

void callOK() {
  writeSerial(0xff);
  writeSerial(0x55);
  writeEnd();
}

void sendByte(char c) {
  writeSerial(1);
  writeSerial(c);
}

void sendString(String s) {
  int l = s.length();
  writeSerial(4);
  writeSerial(l);
  for (int i = 0; i < l; i++) {
    writeSerial(s.charAt(i));
  }
}

// 1 byte 2 float 3 short 4 len+string 5 double
void sendFloat(float value) {
  writeSerial(2);
  val.floatVal = value;
  writeSerial(val.byteVal[0]);
  writeSerial(val.byteVal[1]);
  writeSerial(val.byteVal[2]);
  writeSerial(val.byteVal[3]);
}

void sendShort(double value) {
  writeSerial(3);
  valShort.shortVal = value;
  writeSerial(valShort.byteVal[0]);
  writeSerial(valShort.byteVal[1]);
  writeSerial(valShort.byteVal[2]);
  writeSerial(valShort.byteVal[3]);
}

void sendDouble(double value) {
  writeSerial(5);
  valDouble.doubleVal = value;
  writeSerial(valDouble.byteVal[0]);
  writeSerial(valDouble.byteVal[1]);
  writeSerial(valDouble.byteVal[2]);
  writeSerial(valDouble.byteVal[3]);
  writeSerial(valDouble.byteVal[4]);
  writeSerial(valDouble.byteVal[5]);
  writeSerial(valDouble.byteVal[6]);
  writeSerial(valDouble.byteVal[7]);
}

short readShort(int idx) {
  valShort.byteVal[0] = readBuffer(idx);
  valShort.byteVal[1] = readBuffer(idx + 1);
  return valShort.shortVal;
}

float readFloat(int idx) {
  val.byteVal[0] = readBuffer(idx);
  val.byteVal[1] = readBuffer(idx + 1);
  val.byteVal[2] = readBuffer(idx + 2);
  val.byteVal[3] = readBuffer(idx + 3);
  return val.floatVal;
}

char _receiveStr[20] = {};
uint8_t _receiveUint8[16] = {};
char* readString(int idx, int len) {
  for (int i = 0; i < len; i++) {
    _receiveStr[i] = readBuffer(idx + i);
  }
  _receiveStr[len] = '\0';
  return _receiveStr;
}

uint8_t* readUint8(int idx, int len) {
  for (int i = 0; i < len; i++) {
    if (i > 15) {
      break;
    }
    _receiveUint8[i] = readBuffer(idx + i);
  }
  return _receiveUint8;
}

void runModule(int device) {
  // 0xff 0x55 0x6 0x0 0x2 0x22 0x9 0x0 0x0 0xa
  int port = readBuffer(6);
  int pin = port;
  switch (device) {
    case SERVO: {
      int slot = readBuffer(7);
      pin = slot == 1 ? mePort[port].s1 : mePort[port].s2;
      int v = readBuffer(8);
      Servo sv = servos[searchServoPin(pin)];
      if (v >= 0 && v <= 180) {
        if (!sv.attached()) {
          sv.attach(pin);
        }
        sv.write(v);
      }
    } break;
    case DIGITAL: {
      pinMode(pin, OUTPUT);
      int v = readBuffer(7);
      digitalWrite(pin, v);
    } break;
    case PWM: {
      pinMode(pin, OUTPUT);
      int v = readBuffer(7);
      analogWrite(pin, v);
    } break;
    case TONE: {
      int hz = readShort(6);
      int tone_time = readShort(8);
      if (hz > 0) {
        buzzer.tone(hz, tone_time);
      } else {
        buzzer.noTone();
      }
    } break;
    case SERVO_PIN: {
      int v = readBuffer(7);
      Servo sv = servos[searchServoPin(pin)];
      if (v >= 0 && v <= 180) {
        if (!sv.attached()) {
          sv.attach(pin);
        }
        sv.write(v);
      }
    } break;
    case TIMER: {
      lastTime = millis() / 1000.0;
    } break;
  }
}

int searchServoPin(int pin) {
  for (int i = 0; i < 8; i++) {
    if (servo_pins[i] == pin) {
      return i;
    }
    if (servo_pins[i] == 0) {
      servo_pins[i] = pin;
      return i;
    }
  }
  return 0;
}

void readSensor(int device) {
  /**************************************************
      ff    55      len idx action device port slot data a
      0     1       2   3   4      5      6    7    8
      0xff  0x55   0x4 0x3 0x1    0x1    0x1  0xa
  ***************************************************/
  float value = 0.0;
  int port, pin;
  port = readBuffer(6);
  pin = port;
  switch (device) {
    case ULTRASONIC_SENSOR: {
      if (us.getPort() != port) {
        us.reset(port);
      }
      value = (float)us.distanceCm();
      writeHead();
      writeSerial(command_index);
      sendFloat(value);
    } break;
    case VERSION: {
      sendString(mVersion);
    } break;
    case DIGITAL: {
      pinMode(pin, INPUT);
      sendFloat(digitalRead(pin));
    } break;
    case ANALOG: {
      // pin = analogs[pin];
      pin = pgm_read_byte(&analogs[pin]);
      pinMode(pin, INPUT);
      sendFloat(analogRead(pin));
    } break;
    case TIMER: {
      sendFloat(currentTime);
    } break;
  }
}

void setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(300);
  digitalWrite(13, LOW);
  Serial.begin(115200);
  delay(500);
  buzzer.tone(500, 50);
  delay(50);
  buzzerOff();

  Serial.print("Version: ");
  Serial.println(mVersion);
}

void loop() {
  currentTime = millis() / 1000.0 - lastTime;
  readSerial();
  if (isAvailable) {
    unsigned char c = serialRead & 0xff;
    if (c == 0x55 && isStart == false) {
      if (prevc == 0xff) {
        index = 1;
        isStart = true;
      }
    } else {
      prevc = c;
      if (isStart) {
        if (index == 2) {
          dataLen = c;
        } else if (index > 2) {
          dataLen--;
        }
        writeBuffer(index, c);
      }
    }
    index++;
    if (index > 51) {
      index = 0;
      isStart = false;
    }
    if (isStart && dataLen == 0 && index > 3) {
      isStart = false;
      parseData();
      index = 0;
    }
  }
}
