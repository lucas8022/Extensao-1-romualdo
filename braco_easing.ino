#include <Arduino.h>
#include <ServoEasing.hpp> 
#include "RobotArm.h"

// --- DEFINIÇÃO DOS PINOS ---
const int PIN_START = 8;
const int PIN_STOP = 9;
const int PIN_EMERGENCIA = 10;
const int PIN_SENSOR = 11; 

// --- CONFIGURAÇÃO DOS PINOS DOS SERVOS
RobotArm robot(2, 3, 4, 5, 6, 7);

// --- ESTADOS DO SISTEMA ---
enum EstadoSistema {
    EM_ESPERA,      
    AGUARDANDO_PECA,
    EXECUTANDO,     
    PARANDO,        
    EMERGENCIA      
};

EstadoSistema estadoAtual = EM_ESPERA;

// --- VARIÁVEIS DE CONTROLE ---
int faseCiclo = 0;
unsigned long tempoInicioEspera = 0;
bool emPausaDelay = false;
unsigned long duracaoDelay = 0;

// Declaração de funções
void lerBotoes();
void executarLogicaEsteira();
void iniciarDelay(unsigned long ms);
void gerenciarEstados();

void setup() {
    Serial.begin(115200);
    
    // Configura botões e sensor
    pinMode(PIN_START, INPUT_PULLUP);
    pinMode(PIN_STOP, INPUT_PULLUP);
    pinMode(PIN_EMERGENCIA, INPUT_PULLUP);
    pinMode(PIN_SENSOR, INPUT_PULLUP); 

    // 1. Configurações de Posição
    robot.setParkPose(-90, -90, -90, -90, -90, -90); 
    robot.setMachineZero(0, 0, -90, 0, 0, -90);
    
    // Posição de ESPERA (Acima da esteira, garra aberta)
    robot.setWorkZero(10, -45, -60, 10, 30, -15); // Nota: Garra -15 (aberta)
    
    robot.setSpeed(30);

    // 2. Inicialização
    Serial.println(F("Sistema Iniciando..."));
    robot.begin(); 
    delay(500);

    Serial.println(F("Indo para Home..."));
    robot.goHome();
    
    while(robot.isMoving()) { delay(10); }
    
    Serial.println(F("Pronto. Aguardando START."));
    estadoAtual = EM_ESPERA;
}

void loop() {
    lerBotoes();
    gerenciarEstados();
}

void lerBotoes() {
    // EMERGÊNCIA 
    if (digitalRead(PIN_EMERGENCIA) == LOW) {
        if (estadoAtual != EMERGENCIA) {
            Serial.println(F("!!! EMERGÊNCIA !!!"));
            robot.stopAll();
            robot.detachAll();
            estadoAtual = EMERGENCIA;
        }
    }

    // STOP (Parada Controlada)
    if (digitalRead(PIN_STOP) == LOW) {
        if (estadoAtual == AGUARDANDO_PECA || estadoAtual == EXECUTANDO) {
            Serial.println(F("STOP. Finalizando e voltando para Home..."));
            // Se estiver movendo, para.
            robot.stopAll();
            robot.attachAll();
            robot.setSpeed(30);
            robot.goHome();
            estadoAtual = PARANDO;
        }
    }

    // START 
    if (digitalRead(PIN_START) == LOW) {
        if (estadoAtual != AGUARDANDO_PECA && estadoAtual != EXECUTANDO) {
            Serial.println(F("Sistema Ativado: Indo para Esteira..."));
            robot.attachAll(); // Garante motores ligados
            
            // Recupera de emergência
            if (estadoAtual == EMERGENCIA) {
                robot.goHome();
                while(robot.isMoving()); 
            }
            
            faseCiclo = 0;
            emPausaDelay = false;
            robot.setSpeed(25);
            // Começa o ciclo indo para a posição de espera na esteira
            estadoAtual = EXECUTANDO; 
            delay(300); 
        }
    }
}

void gerenciarEstados() {
    switch (estadoAtual) {
        case EM_ESPERA: 
            // Não faz nada, aguarda START
            break;
            
        case EMERGENCIA: 
            // Não faz nada, aguarda START para resetar
            break;
            
        case PARANDO:
            if (!robot.isMoving()) {
                Serial.println(F("Parado em Home."));
                estadoAtual = EM_ESPERA;
            }
            break;
            
        case EXECUTANDO:
        case AGUARDANDO_PECA: // Ambos usam a mesma função lógica
            executarLogicaEsteira();
            break;
    }
}

void executarLogicaEsteira() {
    // 1. Se o robô está movendo, sai e espera terminar
    if (robot.isMoving()) return;

    // 2. Se existe um delay programado (ex: tempo de fechar a garra), espera
    if (emPausaDelay) {
        if (millis() - tempoInicioEspera >= duracaoDelay) {
            emPausaDelay = false;
        } else {
            return;
        }
    }

    RobotPose deposito = {-90, 10, -30, 0, 0, -90}; // Garra -90 (fechada)
    RobotPose subir    = {10, -45, -20, 10, 30, -90}; 

    // 3. Ciclo de Trabalho
    switch (faseCiclo) {
        case 0:
            // Vai para posição de espera na esteira (WorkZero)
            // Certifique-se que WorkZero tem a garra ABERTA ou adicione comando aqui
            robot.goToWorkZero(); 
            // Abre a garra preventivamente enquanto vai
            robot.moveClaw(-15); 
            
            Serial.println(F("Indo para esteira. Aguardando objeto..."));
            estadoAtual = AGUARDANDO_PECA; // Atualiza status para debug
            faseCiclo++;
            break;

        case 1:
            // Fica "preso" nesta fase em loop até o sensor detectar algo
            
            if (digitalRead(PIN_SENSOR) == HIGH) { // HIGH = Objeto detectado
                Serial.println(F("Objeto Detectado! Pegando..."));
                estadoAtual = EXECUTANDO;
                
                // Pequeno delay para garantir que o objeto está centralizado na garra
                // Se a esteira for rápida, diminua este valor.
                iniciarDelay(500); 
                faseCiclo++;
            }
            break;

        case 2:
            // Fecha a garra
            robot.moveClaw(-90);
            iniciarDelay(800); // Tempo para garantir 
            faseCiclo++;
            break;

        case 3:
            // Levanta/Vai para Depósito
            Serial.println(F("Levando ao deposito..."));
            robot.moveToPose(deposito);
            faseCiclo++;
            break;

        case 4:
            // Abre a garra (Solta a peça)
            Serial.println(F("Soltando..."));
            robot.moveClaw(-15);
            iniciarDelay(500);
            faseCiclo++;
            break;

        case 5:
            robot.moveToPose(subir);
            // robot.moveClaw(-90); // Opcional: fechar garra para o retorno
            faseCiclo++;
            break;

        case 6:
            Serial.println(F("Ciclo concluido. Retornando para esteira."));
            faseCiclo = 0; 
            break;
    }
}

void iniciarDelay(unsigned long ms) {
    emPausaDelay = true;
    tempoInicioEspera = millis();
    duracaoDelay = ms;
}