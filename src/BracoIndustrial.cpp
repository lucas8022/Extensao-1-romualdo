#include "BracoIndustrial.h"

BracoIndustrial::BracoIndustrial(int p1, int p2, int p3, int p4, int p5) {
    pinos[0] = p1; pinos[1] = p2; pinos[2] = p3;
    pinos[3] = p4; pinos[4] = p5;

    // Valores padrao iniciais (pode ser sobrescrito depois)
    setZeroMaquina(90, 90, 90, 90, 90);
    setZeroPeca(90, 90, 90, 90, 90);
    speedGlobal = 30; 
}

void BracoIndustrial::begin() {
    for (int i = 0; i < 5; i++) {
        // Attach com calibracao em micros e limites em graus
        if (servos[i].attach(
                pinos[i],
                zeroMaquina.angles[i],
                SERVO_MIN_MICROS,    // 900us
                SERVO_MAX_MICROS,     // 2100us
                0,
                180
            ) == INVALID_SERVO) {
            Serial.println("Erro servo");
        }
        
        // Define suavizacao do movimento
        servos[i].setEasingType(EASE_QUADRATIC_IN_OUT); 
    }
}

void BracoIndustrial::setZeroMaquina(int a1, int a2, int a3, int a4, int a5) {
    zeroMaquina = {a1, a2, a3, a4, a5};
}

void BracoIndustrial::setZeroPeca(int a1, int a2, int a3, int a4, int a5) {
    zeroPeca = {a1, a2, a3, a4, a5};
}

void BracoIndustrial::setSpeed(int degreesPerSecond) {
    speedGlobal = degreesPerSecond;
}

void BracoIndustrial::attachAll() {
    for (int i = 0; i < 5; i++) {
        if (!servos[i].attached()) {
            // Reatacha usando a calibracao atual
            servos[i].attach(
                pinos[i],
                zeroMaquina.angles[i],
                SERVO_MIN_MICROS, 
                SERVO_MAX_MICROS,
                0,
                180
            );
        }
    }
}

void BracoIndustrial::detachAll() {
    for (int i = 0; i < 5; i++) {
        servos[i].detach();
    }
}

void BracoIndustrial::stopAll() {
    stopAllServos(); 
}


void BracoIndustrial::moveToPose(RobotPose pose) {
    uint16_t speedInt = (uint16_t)speedGlobal;

    for (int i = 0; i < 5; i++) {
        servos[i].setEaseTo(pose.angles[i], speedInt); // Non-Blocking
        //servos[i].easeTo(pose.angles[i], speedInt); // Blocking
    }
    
    // Sincroniza e inicia o movimento
    synchronizeAllServosAndStartInterrupt();
}


void BracoIndustrial::goHome() {
    moveToPose(zeroMaquina);
}

void BracoIndustrial::goToZeroPeca() {
    moveToPose(zeroPeca);
}


void BracoIndustrial::moveGarra(int angle) {
    uint16_t speedInt = (uint16_t)speedGlobal;
    // Movimento isolado da garra
    servos[4].startEaseTo(angle, speedInt);
}

void BracoIndustrial::move3(int angle) {
    uint16_t speedInt = (uint16_t)speedGlobal;
    // Movimento isolado do servo 3
    servos[2].startEaseTo(angle, speedInt);

}

void BracoIndustrial::moveBase(int angle) {
    uint16_t speedInt = (uint16_t)speedGlobal;
    // Movimento isolado da base
    servos[0].startEaseTo(angle, speedInt);

}

    
bool BracoIndustrial::isMoving() {
    // Retorna se as interrupcoes de movimento estao ativas
    return ServoEasing::areInterruptsActive();
}
