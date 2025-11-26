#include <Servo.h>

class ServoJoint {
private:
    Servo servo;
    int pin;
    int minAngle;
    int maxAngle;
    int offset;
    int currentAngle;

public:
    ServoJoint(int pin, int minAngle = 0, int maxAngle = 180, int offset = 0)
        : pin(pin), minAngle(minAngle), maxAngle(maxAngle), offset(offset), currentAngle(90) {}

    void attach() {
        servo.attach(pin);
        servo.write(currentAngle + offset);
    }

    // Movimento com velocidade (vel = delay entre passos)
    void moveTo(int targetAngle, int velocidade = 5) {
        targetAngle = constrain(targetAngle, minAngle, maxAngle);

        if (targetAngle > currentAngle) {
            for (int a = currentAngle; a <= targetAngle; a++) {
                servo.write(a + offset);
                delay(velocidade); // controla velocidade
            }
        } else {
            for (int a = currentAngle; a >= targetAngle; a--) {
                servo.write(a + offset);
                delay(velocidade);
            }
        }
        currentAngle = targetAngle;
    }

    // Movimento incremental
    void moveBy(int delta, int velocidade = 5) {
        moveTo(currentAngle + delta, velocidade);
    }

    int getAngle() {
        return currentAngle;
    }
};

//Criação dos Servos
// Manipulador de 6 DOF
ServoJoint base(2, 0, 180);
ServoJoint shoulder(3, 10, 170);
ServoJoint elbow(4, 20, 160);
ServoJoint wrist1(5, 0, 180);
ServoJoint wrist2(6, 0, 180);
ServoJoint gripper(7, 30, 120);


void setup() {
    base.attach();
    shoulder.attach();
    elbow.attach();
    wrist1.attach();
    wrist2.attach();
    gripper.attach();

    // Posição inicial
    base.moveTo(90);
    shoulder.moveTo(90);
    elbow.moveTo(90);
    wrist1.moveTo(90);
    wrist2.moveTo(90);
    gripper.moveTo(60);
}


void loop() {

    base.moveTo(0);
    delay(1500);

    shoulder.moveTo(120);
    delay(1500);

    elbow.moveTo(60);
    delay(1500);

    gripper.moveBy(-20);  // fecha mais
    delay(1500);

    base.moveTo(180);
    delay(1500);

    // Volta para o zero máquina
    base.moveTo(90);
    shoulder.moveTo(90);
    elbow.moveTo(90);
    wrist1.moveTo(90);
    wrist2.moveTo(90);
    gripper.moveTo(60);
    delay(1500);
}
