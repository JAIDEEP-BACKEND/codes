#include <AFMotor.h>
AF_DCMotor MotorFR(1);   // Front right motor
AF_DCMotor MotorFL(2);   // Front left motor
AF_DCMotor MotorBL(3);   // Back left motor
AF_DCMotor MotorBR(4);   // Back right motor

const int buzPin = 2;  // Buzzer pin
const int ledPin = A5; // LED pin
int valSpeed = 255;    // Default motor speed

void setup() {
    Serial.begin(9600);
    pinMode(buzPin, OUTPUT);
    pinMode(ledPin, OUTPUT);

    // Set initial motor speed
    MotorFL.setSpeed(valSpeed);
    MotorFR.setSpeed(valSpeed);
    MotorBL.setSpeed(valSpeed);
    MotorBR.setSpeed(valSpeed);

    // Stop all motors at the beginning
    MotorFL.run(RELEASE);
    MotorFR.run(RELEASE);
    MotorBL.run(RELEASE);
    MotorBR.run(RELEASE);
}

void loop() {
    while (Serial.available() > 0) {
        char command = Serial.read(); // Read command from serial
        Serial.println(command);

        switch(command) {
            case 'F':   // Move forward
                SetSpeed(valSpeed);
                MotorFL.run(FORWARD);
                MotorFR.run(FORWARD);
                MotorBL.run(FORWARD);
                MotorBR.run(FORWARD);
                break;

            case 'B':   // Move backward
                SetSpeed(valSpeed);
                MotorFL.run(BACKWARD);
                MotorFR.run(BACKWARD);
                MotorBL.run(BACKWARD);
                MotorBR.run(BACKWARD);
                break;

            case 'R':   // Turn right
                SetSpeed(valSpeed);
                MotorFL.run(FORWARD);
                MotorFR.run(BACKWARD);
                MotorBL.run(FORWARD);
                MotorBR.run(BACKWARD);
                break;

            case 'L':   // Turn left
                SetSpeed(valSpeed);
                MotorFL.run(BACKWARD);
                MotorFR.run(FORWARD);
                MotorBL.run(BACKWARD);
                MotorBR.run(FORWARD);
                break;

            case 'G':   // Forward left
                MotorFL.setSpeed(valSpeed/4);
                MotorBL.setSpeed(valSpeed/4);
                MotorFL.run(FORWARD);
                MotorFR.run(FORWARD);
                MotorBL.run(FORWARD);
                MotorBR.run(FORWARD);
                break;

            case 'H':   // Forward right
                MotorFR.setSpeed(valSpeed/4);
                MotorBR.setSpeed(valSpeed/4);
                MotorFL.run(FORWARD);
                MotorFR.run(FORWARD);
                MotorBL.run(FORWARD);
                MotorBR.run(FORWARD);
                break;

            case 'I':   // Backward left
                MotorFL.setSpeed(valSpeed/4);
                MotorBL.setSpeed(valSpeed/4);
                MotorFL.run(BACKWARD);
                MotorFR.run(BACKWARD);
                MotorBL.run(BACKWARD);
                MotorBR.run(BACKWARD);
                break;

            case 'J':   // Backward right
                MotorFR.setSpeed(valSpeed/4);
                MotorBR.setSpeed(valSpeed/4);
                MotorFL.run(BACKWARD);
                MotorFR.run(BACKWARD);
                MotorBL.run(BACKWARD);
                MotorBR.run(BACKWARD);
                break;

            case 'S':   // Stop all motors
                MotorFL.run(RELEASE);
                MotorFR.run(RELEASE);
                MotorBL.run(RELEASE);
                MotorBR.run(RELEASE);
                break;

            case 'Y':   // Honk horn
                digitalWrite(buzPin, HIGH);
                delay(200);
                digitalWrite(buzPin, LOW);
                delay(80);
                digitalWrite(buzPin, HIGH);
                delay(300);
                digitalWrite(buzPin, LOW);
                break;

            case 'X':   // Turn headlight ON
                digitalWrite(ledPin, HIGH);
                break;

            case 'x':   // Turn headlight OFF
                digitalWrite(ledPin, LOW);
                break;

            // Set motor speed using single characters (0-9)
            case '0': SetSpeed(0); break;
            case '1': SetSpeed(25); break;
            case '2': SetSpeed(50); break;
            case '3': SetSpeed(75); break;
            case '4': SetSpeed(100); break;
            case '5': SetSpeed(125); break;
            case '6': SetSpeed(150); break;
            case '7': SetSpeed(175); break;
            case '8': SetSpeed(200); break;
            case '9': SetSpeed(255); break;
        }
    }
}

// Function to update motor speed
void SetSpeed(int val) {
    valSpeed = val;
    MotorFL.setSpeed(val);
    MotorFR.setSpeed(val);
    MotorBL.setSpeed(val);
    MotorBR.setSpeed(val);
}
