#include <MeMCore.h>
#include <Wire.h>

Me4Button buttonSensor;
MeRGBLed led(0, 32);

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
String mVersion = "06.01.107";
boolean isAvailable = false;

int len = 52;
char buffer[52];
byte index = 0;
byte dataLen;
byte modulesLen = 0;
boolean isStart = false;
char serialRead;
uint8_t command_index = 0;
double lastTime = 0.0;
double currentTime = 0.0;

#define VERSION 0
#define RGBLED 8
#define BUTTON 22
#define DIGITAL 30
#define ANALOG 31
#define PWM 32
#define TONE 34
#define BUTTON_INNER 35
#define PULSEIN 37
#define STEPPER 40
#define TIMER 50
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

unsigned char prevc = 0;
boolean buttonPressed = false;
uint8_t keyPressed = KEY_NULL;

/*
 * function list
 */
void readButtonInner(uint8_t pin, int8_t s);
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
void readSensor(int device);

void readButtonInner(uint8_t pin, int8_t s) {
  pin = pgm_read_byte(&analogs[pin]);
  pinMode(pin, INPUT);
  boolean currentPressed = !(analogRead(pin) > 10);

  if (buttonPressed == currentPressed) {
    return;
  }
  buttonPressed = currentPressed;
  writeHead();
  writeSerial(0x80);
  sendByte(currentPressed);
  writeEnd();
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
      readSensor(device);
      writeEnd();
    } break;
    case RUN: {
      runModule(device);
      callOK();
    } break;
    case RESET: {
      callOK();
    } break;
    case START: {
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
    case RGBLED: {
      int slot = readBuffer(7);
      int idx = readBuffer(8);
      int r = readBuffer(9);
      int g = readBuffer(10);
      int b = readBuffer(11);
      if ((led.getPort() != port) || led.getSlot() != slot) {
        led.reset(port, slot);
      }
      if (idx > 0) {
        led.setColorAt(idx - 1, r, g, b);
      } else {
        led.setColor(r, g, b);
      }
      led.show();
    }
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
  int port, slot, pin;
  port = readBuffer(6);
  pin = port;
  switch (device) {
    case BUTTON_INNER: {
      // pin = analogs[pin];
      pin = pgm_read_byte(&analogs[pin]);
      char s = readBuffer(7);
      pinMode(pin, INPUT);
      boolean currentPressed = !(analogRead(pin) > 10);
      sendByte(s ^ (currentPressed ? 1 : 0));
      buttonPressed = currentPressed;
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
    case BUTTON: {
      if (buttonSensor.getPort() != port) {
        buttonSensor.reset(port);
      }
      sendByte(keyPressed == readBuffer(7));
    } break;
  }
}

void setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(300);
  digitalWrite(13, LOW);
  Serial.begin(115200);
  Serial.print("Version: ");
  Serial.println(mVersion);
  led.setpin(13);
  led.setColor(0, 0, 0);
  led.show();
}

void loop() {
  readButtonInner(7, 0);
  keyPressed = buttonSensor.pressed();
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
