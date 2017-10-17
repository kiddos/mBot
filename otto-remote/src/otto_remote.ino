#include <Servo.h>
#include <SoftwareSerial.h>

#define C7 2093
#define B6 1975
#define A6_SHARP 1864
#define A6 1760
#define G6_SHARP 1661
#define G6 1567
#define F6_SHARP 1479
#define F6 1396
#define E6 1318
#define D6_SHARP 1244
#define D6 1174
#define C6_SHARP 1108
#define C6 1046
#define B5 987
#define A5_SHARP 932
#define A5 880
#define G5_SHARP 830
#define G5 783
#define F5_SHARP 739
#define F5 698
#define E5 659
#define D5_SHARP 622
#define D5 587
#define C5_SHARP 554
#define C5 523
#define B4 493
#define A4_SHARP 466
#define A4 440
#define G4_SHARP 415
#define G4 391
#define F4_SHARP 369
#define F4 349
#define E4 329
#define D4_SHARP 311
#define D4 293
#define C4_SHARP 277
#define C4 261

SoftwareSerial bluetooth(1, 0);
Servo servo[4];
bool direction[4] = {false};
union Value {
  char byte_values[4];
  short short_value;
  float float_value;
  double double_value;
} value;

void Pitch(int16_t frequency, uint16_t sleep_millis) {
  tone(8, frequency);
  delay(sleep_millis);
  noTone(8);
}

void DoneMelody() {
  Pitch(F5, 200);
  delay(20);
  Pitch(F5, 200);
  delay(20);
  Pitch(F5, 200);
  delay(220);

  Pitch(E5, 200);
  delay(20);
  Pitch(D5, 200);
  delay(160);
  Pitch(C5, 200);
  delay(20);
}

void StartMelody() {
  Pitch(E5, 200);
  delay(20);
  Pitch(E5, 200);
  delay(60);
  Pitch(E5, 200);
  delay(220);

  Pitch(C5, 200);
  delay(20);
  Pitch(E5, 200);
  delay(160);
  Pitch(G5, 200);
  delay(20);
}

void Reset() {
  bluetooth.begin(115200);
  bluetooth.print("$$$");
  delay(100);
  bluetooth.println("U,9600,N");
  bluetooth.begin(9600);

  int servo_pins[] = {11, 12, A0, A1};
  for (int i = 0; i < 4; ++i) {
    direction[i] = false;
    servo[i].attach(servo_pins[i]);
    delay(500);
  }

  StartMelody();
  OutputMessage("Hi I'm Otto Robot\n");
}

void Walk(int max1, int max2, int paddle_pose[4]) {
  servo[0].write(paddle_pose[0]);
  servo[2].write(paddle_pose[1]);

  const int delta = 2;
  const int max_delta = 30;
  const int sleep_millis = 66;
  for (int16_t i = 0; i < max_delta; i += delta) {
    servo[1].write(max1 - i);
    servo[3].write(max2 - i);
    delay(sleep_millis);
  }

  servo[0].write(paddle_pose[2]);
  servo[2].write(paddle_pose[3]);
  for (int16_t i = max_delta; i > 0; i -= delta) {
    servo[1].write(max1 - i);
    servo[3].write(max2 - i);
    delay(sleep_millis);
  }
}

void Action() {
  if (direction[0]) {
    int paddle_pose[4] = {90, 105, 75, 90};
    Walk(90, 110, paddle_pose);
  } else if (direction[1]) {
    int paddle_pose[4] = {75, 90, 90, 105};
    Walk(120, 90, paddle_pose);
  }
}

char ReadBlocking() {
  while (!bluetooth.available());
  return bluetooth.read();
}

// Code:
// 0: Reset
// 1: Stop
// 2: Forward
// 3: Backward
// 4: Left
// 5: Right
// 6: Buzzer
// 7: Ultrasonic
void ParseCode() {
  if (bluetooth.available()) {
    char code = bluetooth.read();
    int16_t freq;
    uint16_t sleep_time;

    switch (code) {
      case 0:
        bluetooth.end();
        DoneMelody();

        delay(1000);
        Reset();
        break;
      case 1:
        for (int i = 0; i < 4; ++i) {
          direction[i] = false;
        }
        break;
      case 2:
        direction[0] = true;
        break;
      case 3:
        direction[1] = true;
        break;
      case 4:
        direction[2] = true;
        break;
      case 5:
        direction[3] = true;
        break;
      case 6:
        value.byte_values[0] = ReadBlocking();
        value.byte_values[1] = ReadBlocking();
        freq = value.short_value;
        value.byte_values[0] = ReadBlocking();
        value.byte_values[1] = ReadBlocking();
        sleep_time = value.short_value;
        Pitch(freq, sleep_time);
        break;
      case 7:
        break;
    }
  }
}

void OutputMessage(const char* const message) { bluetooth.write(message); }

void setup() {
  pinMode(8, OUTPUT);

  Reset();
}

void loop() {
  ParseCode();
  Action();
}
