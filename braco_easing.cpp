#include "braco_easing.h"

// Construtor: Apenas salva os pinos e define valores padrão
RobotArm::RobotArm(int p1, int p2, int p3, int p4, int p5, int p6) {
    pinos[0] = p1; pinos[1] = p2; pinos[2] = p3;
    pinos[3] = p4; pinos[4] = p5; pinos[5] = p6;

    // Valores padrão de segurança (tudo em 90 graus)
    setZeroMaquina(90, 90, 90, 90, 90, 90);
    setZeroPeca(90, 90, 90, 90, 90, 90);
    setParkPose(90, 90, 90, 90, 90, 90);
    speedGlobal = 30; 
}

void RobotArm::begin() {
    // Inicializa cada servo individualmente
    for (int i = 0; i < 6; i++) {
        // Tenta anexar. O segundo parâmetro é a posição inicial (Park)
        // Isso evita que o robô dê um tranco ao ligar, pois ele inicia onde "deveria" estar.
        if (servos[i].attach(pinos[i], parkPose.angles[i]) == INVALID_SERVO) {
             // Tratamento de erro (opcional: piscar LED)
        }
        // Define o tipo de movimento suave (Cúbico é o mais natural para braços)
        servos[i].setEasingType(EASE_CUBIC_IN_OUT); 
    }
}

// --- Setters de Posição ---
void RobotArm::setZeroMaquina(int a1, int a2, int a3, int a4, int a5, int a6) {
    zeroMaquina = {a1, a2, a3, a4, a5, a6};
}

void RobotArm::setZeroPeca(int a1, int a2, int a3, int a4, int a5, int a6) {
    zeroPeca = {a1, a2, a3, a4, a5, a6};
}

void RobotArm::setParkPose(int a1, int a2, int a3, int a4, int a5, int a6) {
    parkPose = {a1, a2, a3, a4, a5, a6};
}

void RobotArm::setSpeed(float degreesPerSecond) {
    speedGlobal = degreesPerSecond;
}

// --- Controle de Hardware ---
void RobotArm::attachAll() {
    for (int i = 0; i < 6; i++) {
        if (!servos[i].attached()) {
            // Ao reanexar, usa a posição atual lida para evitar pulos
            servos[i].attach(pinos[i], servos[i].getCurrentAngle());
        }
    }
}

void RobotArm::detachAll() {
    for (int i = 0; i < 6; i++) {
        servos[i].detach();
    }
}

void RobotArm::stopAll() {
    // Função GLOBAL da biblioteca ServoEasing (não pertence à classe ServoEasing)
    // Para todos os servos instantaneamente.
    stopAllServos(); 
}

// --- Movimentos Sincronizados ---

void RobotArm::moveToPose(RobotPose pose) {
    // A biblioteca espera uint16_t para velocidade
    uint16_t speedInt = (uint16_t)speedGlobal;

    // 1. "Agenda" o movimento de cada servo. 
    // setEaseTo NÃO move o servo, apenas configura o destino.
    for (int i = 0; i < 6; i++) {
        servos[i].setEaseTo(pose.angles[i], speedInt);
    }
    
    // 2. Inicia o movimento de TODOS ao mesmo tempo.
    // A biblioteca calcula quem vai demorar mais e sincroniza os outros.
    // Esta é uma função GLOBAL da biblioteca.
    synchronizeAllServosAndStartInterrupt();
}

void RobotArm::goHome() {
    moveToPose(zeroMaquina);
}

void RobotArm::goToZeroPeca() {
    moveToPose(zeroPeca);
}

void RobotArm::goToPark() {
    moveToPose(parkPose);
}

void RobotArm::moveClaw(int angle) {
    uint16_t speedInt = (uint16_t)speedGlobal;
    // Para mover um único servo imediatamente, usamos startEaseTo
    // O último servo é o índice 5 (0 a 5 = 6 servos)
    servos[5].startEaseTo(angle, speedInt);
}

bool RobotArm::isMoving() {
    // Verifica se a interrupção de movimento está ativa
    return ServoEasing::areInterruptsActive();
}