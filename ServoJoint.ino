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
    // Construtor
    ServoJoint(int pin, int minAngle = 0, int maxAngle = 180, int offset = 0)
        : pin(pin), minAngle(minAngle), maxAngle(maxAngle), offset(offset), currentAngle(90) {}

    // Anexa o servo ao pino
    void attach() {
        servo.attach(pin);
        servo.write(currentAngle + offset);
    }

    // Mover para ângulo absoluto
    void moveTo(int angle) {
        angle = constrain(angle, minAngle, maxAngle);
        currentAngle = angle;
        servo.write(angle + offset);
    }

    // Incremental
    void moveBy(int delta) {
        moveTo(currentAngle + delta);
    }

    // Ler ângulo atual
    int getAngle() {
        return currentAngle;
    }

    // Definir offset no eixo
    void setOffset(int o) {
        offset = o;
        servo.write(currentAngle + offset);
    }
};
