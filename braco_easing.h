#ifndef ROBOTARM_H
#define ROBOTARM_H

#include <Arduino.h>
// Inclui apenas o Header básico aqui
#include <ServoEasing.h> 

struct RobotPose {
    int angles[6];
};

class RobotArm {
    private:
        ServoEasing servos[6]; // Array de objetos ServoEasing
        int pinos[6];          // Pinos físicos onde estão conectados
        
        RobotPose zeroMaquina; // Posição Zero Máquina (Home)
        RobotPose zeroPeca;    // Posição Zero Peça
        RobotPose parkPose;    // Posição de descanso (para inicialização suave)

        float speedGlobal;     // Velocidade padrão (graus/segundo)

    public:
        // Construtor
        RobotArm(int p1, int p2, int p3, int p4, int p5, int p6);

        // Inicialização
        void begin(); 

        // Configurações de Coordenadas
        void setZeroMaquina(int a1, int a2, int a3, int a4, int a5, int a6);
        void setZeroPeca(int a1, int a2, int a3, int a4, int a5, int a6);
        void setParkPose(int a1, int a2, int a3, int a4, int a5, int a6);
        void setSpeed(float degreesPerSecond);

        // Controle de Estado dos Motores
        void attachAll();       
        void detachAll();       
        void stopAll();         

        // Movimentos
        void goHome();
        void goToZeroPeca();
        void goToPark();
        void moveToPose(RobotPose pose);
        
        // Controle Manual da Garra (Servo 6)
        void moveClaw(int angle);

        // Verifica status
        bool isMoving();
};

#endif