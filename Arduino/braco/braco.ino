#include <ServoJoint.h>
//#include <VarSpeedServo.h>


//Criação dos Servos
ServoJoint servo1(5, 0, 180);         // SERVO 1

void setup() {
    Serial.begin(9600);
    
    // Inicializa o servo na posição 0
    servo1.attach();
    delay(1000);
}

void loop() {

    servo1.moveTo(0, 30, true); // Speed 30, Wait = true
    delay(500);

    servo1.moveTo(90, 30, true);
    delay(500);

    // Exemplo de movimento incremental
    servo1.moveBy(20, 10, true); // Vai para 110
    delay(1000);

}
