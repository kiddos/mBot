#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "mCore.h"

MeUltrasonic ultr(PORT_3);
MeBuzzer buzzer;
MePort generalDevice;
Servo servo;

#define NTD1 294
#define NTD2 330
#define NTD3 350
#define NTD4 393
#define NTD5 441
#define NTD6 495
#define NTD7 556
#define NTDL1 147
#define NTDL2 165
#define NTDL3 175
#define NTDL4 196
#define NTDL5 221
#define NTDL6 248
#define NTDL7 278
#define NTDH1 589
#define NTDH2 661
#define NTDH3 700
#define NTDH4 786
#define NTDH5 882
#define NTDH6 990
#define NTDH7 112

#define RUN_F 0x01
#define RUN_B 0x01 << 1
#define RUN_L 0x01 << 2
#define RUN_R 0x01 << 3
#define STOP 0
uint8_t motor_sta = STOP;
enum { MODE_A, MODE_B, MODE_C };

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

MeModule modules[12];
int analogs[8] = {A0, A1, A2, A3, A4, A5, A6, A7};
uint8_t mode = MODE_A;

boolean isAvailable = false;
int len = 52;
char buffer[52];
char bufferBt[52];
byte index = 0;
byte dataLen;
byte modulesLen = 0;
boolean isStart = false;
char serialRead;
String mVersion = "1.2.103";
float angleServo = 90.0;
unsigned char prevc = 0;
double lastTime = 0.0;
double currentTime = 0.0;

#define VERSION 0
#define ULTRASONIC_SENSOR 1
#define SERVO 11
#define SHUTTER 20
#define DIGITAL 30
#define ANALOG 31
#define PWM 32
#define SERVO_PIN 33
#define TONE 34
#define TIMER 50

#define GET 1
#define RUN 2
#define RESET 4
#define START 5

void setup() {
  buzzer.tone(NTD1, 300);
  delay(300);
  buzzer.tone(NTD2, 300);
  delay(300);
  buzzer.tone(NTD3, 300);
  delay(300);
  Serial.begin(115200);
  buzzer.noTone();
}

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

void serialHandle() {
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

int px = 0;
void loop() {
  while (true) {
    serialHandle();
  }
}

void parseData() {
  isStart = false;
  int idx = readBuffer(3);
  int action = readBuffer(4);
  int device = readBuffer(5);
  switch (action) {
    case GET: {
      writeHead();
      writeSerial(idx);
      readSensor(device);
      writeEnd();
    } break;
    case RUN: {
      runModule(device);
      callOK();
    } break;
    case RESET: {
      // reset
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
      if (v >= 0 && v <= 180) {
        servo.attach(pin);
        servo.write(v);
      }
    } break;
    case SHUTTER: {
      if (generalDevice.getPort() != port) {
        generalDevice.reset(port);
      }
      int v = readBuffer(7);
      if (v < 2) {
        generalDevice.dWrite1(v);
      } else {
        generalDevice.dWrite2(v - 2);
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
      pinMode(pin, OUTPUT);
      int hz = readShort(6);
      if (hz > 0) {
        buzzer.tone(hz);
      } else {
        buzzer.noTone();
      }
    } break;
    case SERVO_PIN: {
      int v = readBuffer(7);
      if (v >= 0 && v <= 180) {
        servo.attach(pin);
        servo.write(v);
      }
    } break;
    case TIMER: {
      lastTime = millis() / 1000.0;
    } break;
  }
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
      if (ultr.getPort() != port) {
        ultr.reset(port);
      }
      value = (float)ultr.distanceCm(50000);
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
      pin = analogs[pin];
      pinMode(pin, INPUT);
      sendFloat(analogRead(pin));
    } break;
    case TIMER: {
      sendFloat(currentTime);
    } break;
  }
}
