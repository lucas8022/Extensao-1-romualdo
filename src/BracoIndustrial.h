#ifndef BRACOINDUSTRIAL_H
#define BRACOINDUSTRIAL_H

#include <Arduino.h>
#include <ServoEasing.h> 

// --- CONSTANTES DE CALIBRAÇÃO DOS SERVOS ---

#define SERVO_MIN_MICROS      500  // Valor de pulso para 0 graus (900)
#define SERVO_MAX_MICROS      2500 // Valor de pulso para 180 graus (2100)
#define SERVO_TRIM_DEGREE     90   // Ponto onde o servo deve ter 1500us (Centro)

// Conjunto de angulos (graus) para os 5 servos
struct RobotPose {
    int angles[5];
};

// Classe de controle do braco com 5 servos
class BracoIndustrial {
    private:
        // Servos e pinos associados
        ServoEasing servos[5]; 
        int pinos[5];          
        
        // Poses de referencia
        RobotPose zeroMaquina; 
        RobotPose zeroPeca;       

        // Velocidade global (graus/segundo)
        int speedGlobal;     

    public:
        // Construtor com os pinos dos 5 servos
        BracoIndustrial(int p1, int p2, int p3, int p4, int p5);

        // Inicializa os servos e parametros de easing
        void begin(); 

        // Calibracoes das poses
        void setZeroMaquina(int a1, int a2, int a3, int a4, int a5);
        void setZeroPeca(int a1, int a2, int a3, int a4, int a5);
        // Define velocidade de movimento
        void setSpeed(int degreesPerSecond);

        // Controle de attach/detach
        void attachAll();       
        void detachAll();       
        void stopAll();         

        // Movimentos
        void goHome();
        void goToZeroPeca();
        void moveToPose(RobotPose pose);
        void moveGarra(int angle);
        void moveBase(int angle);
        void move3(int angle);

        // Indica se algum servo ainda esta em movimento
        bool isMoving();
};

#endif
