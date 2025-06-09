#include <AFMotor.h>
#include <math.h>

// blinker pins
const uint8_t fr_blink_pin = 2;
const uint8_t rr_blink_pin = 9;
const uint8_t rl_blink_pin = 10;
const uint8_t fl_blink_pin = 13;

const double joyDeadzone = 75;

// mootors
AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

// movement
double x;
double y;
double turn;
double forward;

// siren
bool siren;

// blinker
bool blinker;
const short blink_delay = 100;
unsigned long blinker_timer = 0;
uint8_t blinker_num = 0;
uint8_t blinkers_pins[] = { fr_blink_pin, rr_blink_pin, rl_blink_pin, fl_blink_pin };

void setup() {
  Serial.begin(115200);

  // make every blinker pin to output
  for (int thisPin = 0; thisPin < 4; thisPin++) {
    pinMode(blinkers_pins[thisPin], OUTPUT);
  }
}

void loop() {
  // get commands and controll movement
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');  // Читаємо до кінця рядка
    Serial.println(data);
    int firstComma = data.indexOf(',');

    if (data[0] == '!') {
      String command = data.substring(0, firstComma);
      String valueStr = data.substring(firstComma + 1);
      bool value = (valueStr == "1" || valueStr == "true");

      if (command == "!blinker") {
        blinker = value;
      }

      if (command == "!siren") {
        siren = value;
      }
    }

    int secondComma = data.indexOf(',', firstComma + 1);

    if (firstComma != -1 && secondComma != -1) {
      String joyID = data.substring(0, firstComma);
      int joyX = data.substring(firstComma + 1, secondComma).toInt();
      int joyY = data.substring(secondComma + 1).toInt();
      Serial.print("X: ");
      Serial.print(joyX);
      Serial.print("; Y: ");
      Serial.println(joyY);

      if (joyID == "left-joy") {
        // Керування кухами вперед-назад і вліво-вправо
        x = static_cast<double>(joyX) / -100.0;
        y = static_cast<double>(joyY) / 100.0;
        crabMovement(x, y);
      } else if (joyID == "right-joy") {
        // Керування поворотом
        turn = static_cast<double>(joyX) / 100.0;
        forward = static_cast<double>(joyY) / 100.0;
        defaultMovement(forward, turn);
      }
    }
  }

  // blinker
  if (blinker) {
    const long time_from_last_blink = millis() - blinker_timer;
    if (time_from_last_blink > blink_delay) {
      // turn on current blinker
      digitalWrite(blinkers_pins[blinker_num], HIGH);

      // turn off previous blinker
      uint8_t prev_led = (blinker_num == 0) ? 3 : blinker_num - 1;
      digitalWrite(blinkers_pins[prev_led], LOW);

      // go to next blinker led
      blinker_num = (blinker_num + 1) % 4;

      // update timer ONLY after blink
      blinker_timer = millis();
    }
  } else {
    // turn off all blinkers
    for (uint8_t i = 0; i < 4; i++) {
      digitalWrite(blinkers_pins[i], LOW);
    }
  }
}

void defaultMovement(double forward, double turn) {  // forward є (-1.0; 1.0), turn є (-1.0; 1.0)
  double left = forward + turn / 2;
  double right = forward - turn / 2;

  // нормаліхація чисел
  double maxVal = max(max(abs(left), abs(right)), 1.0f);
  left /= maxVal;
  right /= maxVal;

  double leftFront = left;
  double leftRear = left;

  double rightFront = right;
  double rightRear = right;

  setMovement(leftFront, rightFront, leftRear, rightRear);
}

void crabMovement(double x, double y) {
  double theta = atan2(y, x);
  double power = hypot(x, y);

  double Vx = power * sin(theta);
  double Vy = power * cos(theta);

  double leftFront, rightFront, leftRear, rightRear;

  if (power < 0.05) {  // Якщо немає руху (робот стоїть)
    leftFront = turn;
    rightFront = -turn;
    leftRear = turn;
    rightRear = -turn;
  } else {  // Стандартний рух
    leftFront = Vx - Vy;
    rightFront = Vx + Vy;
    leftRear = Vx + Vy;
    rightRear = Vx - Vy;
  }

  setMovement(leftFront, rightFront, leftRear, rightRear);
}

void setMovement(double leftF, double rightF, double leftR, double rightR) {

  int mappedLeftR = constrain((int)(leftR * 255), -255, 255);
  int mappedLeftF = constrain((int)(leftF * 255), -255, 255);
  int mappedRightR = constrain((int)(rightR * 255), -255, 255);
  int mappedRightF = constrain((int)(rightF * 255), -255, 255);

  setMotor(motor1, mappedLeftR);
  setMotor(motor2, mappedLeftF);
  setMotor(motor3, mappedRightR);
  setMotor(motor4, mappedRightF);
}

void setMotor(AF_DCMotor &motor, int speed) {
  if (speed > joyDeadzone) {
    motor.setSpeed(speed);
    motor.run(FORWARD);
  } else if (speed < -joyDeadzone) {
    motor.setSpeed(-speed);
    motor.run(BACKWARD);
  } else {
    motor.run(RELEASE);
    motor.setSpeed(0);
  }
}
