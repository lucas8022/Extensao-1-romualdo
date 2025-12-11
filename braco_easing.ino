/*
 * braco_easing.ino
 * Controle de Braço Robótico com ServoEasing
 * Baseado nos exemplos oficiais da biblioteca ServoEasing.
 */


#include <ServoEasing.hpp> 
#include "braco_easing.h"

// --- PINOS DOS BOTÕES ---
const int PIN_START = 8;
const int PIN_STOP = 9;
const int PIN_EMERGENCIA = 10;

// --- CONFIGURAÇÃO DOS PINOS DOS SERVOS ---
RobotArm robot(2, 3, 4, 5, 6, 7);

// --- ESTADOS DO SISTEMA ---
enum EstadoSistema {
    EM_ESPERA,      // Parado em Home/Park
    TRABALHANDO,    // Executando o ciclo
    PARANDO,        // Solicitado STOP, finalizando movimento
    EMERGENCIA      // Parada total 
};

EstadoSistema estadoAtual = EM_ESPERA;

// --- VARIÁVEIS DE CONTROLE DE FLUXO ---
int faseAtual = 0;
unsigned long tempoInicioEspera = 0;
bool emPausaDelay = false;
unsigned long duracaoDelay = 0;

void setup() {
    Serial.begin(115200);
    
    // Configura botões com Pull-Up interno (acionam com LOW)
    pinMode(PIN_START, INPUT_PULLUP);
    pinMode(PIN_STOP, INPUT_PULLUP);
    pinMode(PIN_EMERGENCIA, INPUT_PULLUP);

    // 1. Configurações de Posição (Ângulos)
    // Park: Posição de "descanso" físico (ex: robô desligado/caído)
    // Isso evita o "pulo" ao ligar o Arduino.
    robot.setParkPose(90, 90, 90, 90, 90, 0); 
    
    // Zero Máquina: Posição "Home" ou de calibração
    robot.setZeroMaquina(90, 90, 90, 90, 90, 0);
    
    // Zero Peça: Onde o trabalho começa
    robot.setZeroPeca(100, 45, 30, 110, 120, 0);
    
    // Define velocidade global (Graus por segundo)
    robot.setSpeed(40);

    // 2. Inicialização do Robô
    Serial.println(F("Inicializando Robô..."));
    
    // O begin anexa os servos na posição Park suavemente
    robot.begin(); 
    
    delay(500);

    // Vai para Home suavemente
    Serial.println(F("Indo para Home..."));
    robot.goHome();
    
    // Aguarda chegar em Home antes de liberar o loop
    while(robot.isMoving()) { 
        // Pequeno delay para não travar totalmente o processador
        delay(10); 
    }
    
    Serial.println(F("Pronto. Aguardando Start."));
    estadoAtual = EM_ESPERA;
}

void loop() {
    lerBotoes();
    gerenciarEstados();
}

void lerBotoes() {
    // --- EMERGÊNCIA (Prioridade Máxima) ---
    if (digitalRead(PIN_EMERGENCIA) == LOW) {
        if (estadoAtual != EMERGENCIA) {
            Serial.println(F("!!! EMERGÊNCIA ACIONADA !!!"));
            robot.stopAll();   // Para movimento instantaneamente
            robot.detachAll(); // Desliga a força dos motores
            estadoAtual = EMERGENCIA;
        }
    }

    // --- STOP (Parada Controlada) ---
    if (digitalRead(PIN_STOP) == LOW) {
        if (estadoAtual == TRABALHANDO) {
            Serial.println(F("STOP pressionado. Voltando para Home..."));
            robot.stopAll();   // Interrompe o movimento atual
            robot.attachAll(); // Garante que motores estão ativos
            robot.setSpeed(30);
            robot.goHome();    // Envia comando para voltar
            estadoAtual = PARANDO;
        }
    }

    // --- START (Iniciar Ciclo) ---
    if (digitalRead(PIN_START) == LOW) {
        if (estadoAtual != TRABALHANDO) {
            Serial.println(F("Iniciando Ciclo Automático..."));
            robot.attachAll(); // Religa motores se estiverem soltos (Emergência)
            
            // Se veio da emergência, move para Home primeiro para garantir referência
            if (estadoAtual == EMERGENCIA) {
                robot.goHome();
                while(robot.isMoving()); 
            }
            
            // Reinicia variáveis do ciclo
            faseAtual = 0;
            emPausaDelay = false;
            robot.setSpeed(40); // Restaura velocidade de trabalho
            estadoAtual = TRABALHANDO;
            delay(300); // Debounce simples
        }
    }
}

void gerenciarEstados() {
    switch (estadoAtual) {
        case EM_ESPERA:
            // Ocioso, motores ligados mantendo posição
            break;

        case EMERGENCIA:
            // Ocioso, motores desligados
            break;

        case PARANDO:
            // Verifica se já terminou de voltar para Home
            if (!robot.isMoving()) {
                Serial.println(F("Robô estacionado em Home."));
                estadoAtual = EM_ESPERA;
            }
            break;

        case TRABALHANDO:
            executarCicloPassoAPasso();
            break;
    }
}

// Lógica de produção "Non-Blocking" (Sem delay travante)
void executarCicloPassoAPasso() {
    
    // 1. Se o robô ainda está se movendo, SAI da função e deixa ele terminar.
    if (robot.isMoving()) return;

    // 2. Se estamos em um tempo de espera (ex: garra fechando), verifica o relógio
    if (emPausaDelay) {
        if (millis() - tempoInicioEspera >= duracaoDelay) {
            emPausaDelay = false; // O tempo acabou, prosseguir
        } else {
            return; // Ainda esperando
        }
    }

    // Definição de Poses Locais (Ângulos para os 6 eixos)
    RobotPose deposito = {180, 100, 40, 90, 90, 75};
    RobotPose subir    = {100, 45, 15, 110, 120, 0};

    // 3. Máquina de Fases do Ciclo
    switch (faseAtual) {
        case 0:
            Serial.println(F("Fase 0: Indo para Zero Peça"));
            robot.goToZeroPeca();
            iniciarDelay(1000); // Espera 1s após chegar
            faseAtual++;
            break;

        case 1:
            Serial.println(F("Fase 1: Fechando Garra"));
            robot.moveClaw(75);
            iniciarDelay(500);
            faseAtual++;
            break;

        case 2:
            Serial.println(F("Fase 2: Movendo para Depósito"));
            robot.moveToPose(deposito);
            iniciarDelay(2000);
            faseAtual++;
            break;

        case 3:
            Serial.println(F("Fase 3: Abrindo Garra"));
            robot.moveClaw(0);
            iniciarDelay(500);
            faseAtual++;
            break;

        case 4:
            Serial.println(F("Fase 4: Subindo/Retornando"));
            robot.moveToPose(subir);
            iniciarDelay(1000);
            faseAtual++;
            break;

        case 5:
            Serial.println(F("Ciclo completo. Reiniciando..."));
            faseAtual = 0; // Loop infinito
            break;
    }
}

// Função auxiliar para configurar a espera sem travar o Arduino
void iniciarDelay(unsigned long ms) {
    emPausaDelay = true;
    tempoInicioEspera = millis();
    duracaoDelay = ms;
}