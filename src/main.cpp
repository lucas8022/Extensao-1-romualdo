#include <Arduino.h>
#include <ServoEasing.hpp> 
#include "BracoIndustrial.h"

// --- DEFINIÇÃO DOS PINOS (I/O) ---
const int PIN_SENSOR_A = 26; // Sensor da Peça (LOW = Objeto detectado)
const int PIN_SENSOR_B = 24; // Sensor do segundo braço 
const int PIN_ESTEIRA  = 22; // Saída: Controle do motor (LOW = LIGA, HIGH = DESLIGA)
const int PIN_START    = 8; 
const int PIN_STOP     = 9;
const int PIN_VAR_AUX  = 28; 

// --- INSTANCIAÇÃO DO BRAÇO ---
BracoIndustrial robot(5, 6, 7, 11, 10); 

// --- ESTADOS DO SISTEMA (FSM GERAL) ---
enum EstadoSistema {
    EM_ESPERA_START, 
    INICIANDO,       
    AGUARDANDO_PECA, 
    EXECUTANDO_CICLO,
    PARANDO,         
    REPOUSO_HOME     
};

EstadoSistema estadoAtual = EM_ESPERA_START;

// --- ESTADOS DO SENSOR B (Lógica de Tempo) ---
enum EstadosB {
    B_MONITORANDO, // 0: Funcionamento normal
    B_BLOQUEIO_5S, // 1: Parado por 5s
    B_IGNORA_3S    // 2: Rodando, ignora sensor por 3s
};

EstadosB estadoSensorB = B_MONITORANDO;
unsigned long timerSensorB = 0;

// --- VARIÁVEIS DE CONTROLE ---
unsigned long tempoUltimaAtividade = 0;
const unsigned long TEMPO_TIMEOUT = 10000; 

bool acordouComObjeto = false; 
bool esteiraDeveRodar = false; // Intenção do robô (Software)

int faseCiclo = 0;
unsigned long tempoInicioDelay = 0;
bool emPausaDelay = false;
unsigned long duracaoDelay = 0;

// Declaração de funções auxiliares
void lerBotoes();
void executarCicloPassoAPasso();
void iniciarDelay(unsigned long ms);
void solicitarEsteira(bool ligar); 
void gerenciarHardwareEsteira();   
void delayComSeguranca(unsigned long ms); 

// Configuracao inicial do sistema
void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_SENSOR_A, INPUT_PULLUP);
    pinMode(PIN_SENSOR_B, INPUT_PULLUP); 
    pinMode(PIN_ESTEIRA, OUTPUT);
    pinMode(PIN_START, INPUT_PULLUP);
    pinMode(PIN_STOP, INPUT_PULLUP);
    pinMode(PIN_VAR_AUX, OUTPUT);
    
    // Calibração
    robot.setZeroMaquina(30, 120, 0, 90, 50);  
    robot.setZeroPeca(150, 85, 60, 97, 75);     
    robot.setSpeed(30); 

    Serial.println(F("Sistema Ligado."));
    robot.begin();
    delayComSeguranca(100);

    Serial.println(F("Indo para Zero Maquina (Home)..."));
    robot.goHome();
    solicitarEsteira(false); 
    
    while(robot.isMoving()) { 
        gerenciarHardwareEsteira(); 
        delay(10); 
    }

    Serial.println(F("PRONTO. Aguardando botao START."));
    estadoAtual = EM_ESPERA_START;
}

// Loop principal
void loop() {
    lerBotoes(); 
    
    gerenciarHardwareEsteira(); 

    switch (estadoAtual) {
        
        case EM_ESPERA_START:
            break;

        case INICIANDO:
            if (!robot.isMoving()) {
                Serial.println(F("Posicionado Zero Peça..."));
                //solicitarEsteira(true); 
                tempoUltimaAtividade = millis(); 
                estadoAtual = AGUARDANDO_PECA;
            }
            break;

        case AGUARDANDO_PECA:
            if (robot.isMoving()) {
                tempoUltimaAtividade = millis();
                gerenciarHardwareEsteira();
                return;
            }

            if (acordouComObjeto) {
                Serial.println(F("Acordou com objeto. Ciclo imediato."));
                solicitarEsteira(false); 
                acordouComObjeto = false; 
                delayComSeguranca(1000); 
                faseCiclo = 0;
                estadoAtual = EXECUTANDO_CICLO;
                return;
            }

            // Sensor A (Peça Chegou) - PRIORIDADE
            if (digitalRead(PIN_SENSOR_A) == LOW) {
                Serial.println(F("Sensor A: Objeto Detectado! Parando esteira."));
                solicitarEsteira(false); 
                delay(130);
                faseCiclo = 0;
                estadoAtual = EXECUTANDO_CICLO;
            }
            // Timeout -> Home
            else if (millis() - tempoUltimaAtividade > TEMPO_TIMEOUT) {
                Serial.println(F("Ocioso. Indo para Home..."));
                robot.move3(0);
                delayComSeguranca(2000); 
                robot.goHome();          
                estadoAtual = REPOUSO_HOME;
            }
            break;

        case REPOUSO_HOME:
            if (robot.isMoving()) {
                gerenciarHardwareEsteira();
                return;
            } 

            if (digitalRead(PIN_SENSOR_A) == LOW) {
                Serial.println(F("Objeto visto (Sensor A)! Parando esteira e indo pegar objeto..."));
                
                solicitarEsteira(false); 
                
                robot.moveBase(140);
                delayComSeguranca(1200);
                robot.goToZeroPeca(); 
                
                tempoUltimaAtividade = millis(); 
                acordouComObjeto = true; 
                
                estadoAtual = AGUARDANDO_PECA;
            }
            break;

        case EXECUTANDO_CICLO:
            executarCicloPassoAPasso();
            break;

        case PARANDO:
            if (!robot.isMoving()) {
                Serial.println(F("Parado em Home. Aguardando START."));
                estadoAtual = EM_ESPERA_START;
            }
            break;
    }
}

void lerBotoes() {
    if (digitalRead(PIN_START) == LOW) {
        if (estadoAtual == EM_ESPERA_START) {
            Serial.println(F("START pressionado!"));
            digitalWrite(PIN_VAR_AUX, HIGH);    // variavel auxiliar HIGH
            robot.attachAll(); 
            robot.moveBase(150);
            delayComSeguranca(2500);
            robot.goToZeroPeca(); 
            estadoAtual = INICIANDO;
            delayComSeguranca(300); 
        }
    }

    if (digitalRead(PIN_STOP) == LOW) {
        if (estadoAtual == AGUARDANDO_PECA || estadoAtual == EXECUTANDO_CICLO || 
            estadoAtual == INICIANDO || estadoAtual == REPOUSO_HOME) {
            
            Serial.println(F("STOP pressionado!"));
            solicitarEsteira(false); 
            digitalWrite(PIN_VAR_AUX, LOW);    // variavel auxiliar HIGH
            robot.stopAll();         
            robot.attachAll();
            robot.move3(0);
            delayComSeguranca(2000);
            robot.goHome();          
            
            emPausaDelay = false;
            estadoAtual = PARANDO;
            delayComSeguranca(300); 
        }
    }
}

void executarCicloPassoAPasso() {
    if (robot.isMoving()) {
        gerenciarHardwareEsteira();
        return;
    }

    if (emPausaDelay) {
        if (millis() - tempoInicioDelay >= duracaoDelay) {
            emPausaDelay = false;
        } else {
            gerenciarHardwareEsteira();
            return;
        }
    }

    RobotPose deposito = {75, 130, 31, 75, 110};
    RobotPose ajuste    = {75, 130, 10, 97, 50}; 

    switch (faseCiclo) {
        case 0:
            Serial.println(F("CASE 0: BAIXANDO BRAÇO"));
            robot.move3(75);
            faseCiclo++;
            break;
        case 1: 
            Serial.println(F("CASE 1: FECHANDO GARRA"));
            robot.moveGarra(114); 
            faseCiclo++;
            break;

        case 2:
            Serial.println(F("CASE 2: SUBINDO BRAÇO"));
            robot.move3(5);
            faseCiclo++;
            break;

        case 3: // Leva para depósito
            Serial.println(F("CASE 3: DEPÓSITO"));
            // solicitarEsteira(true); 
            robot.moveToPose(deposito);
            faseCiclo++;
            break;

        case 4: // Solta
            Serial.println(F("CASE 4: ABRINDO GARRA"));
            robot.moveGarra(50);
            faseCiclo++;
            break;

        case 5: // Ajuste fino
            Serial.println(F("CASE 5: AJUSTANDO"));
            robot.moveToPose(ajuste);
            faseCiclo++;
            break;

        case 6: // Volta para posição de espera
            Serial.println(F("CASE 6 : VOLTANDO P/ ZERO-PECA"));
            robot.goToZeroPeca();
            faseCiclo++;
            break;

        case 7: // Fim do ciclo
            Serial.println(F("Ciclo Fim."));
            tempoUltimaAtividade = millis(); 
            estadoAtual = AGUARDANDO_PECA;   
            break;
    }
}

void iniciarDelay(unsigned long ms) {
    emPausaDelay = true;
    tempoInicioDelay = millis();
    duracaoDelay = ms;
}

// --- CONTROLE INTELIGENTE DA ESTEIRA ---

void solicitarEsteira(bool ligar) {
    esteiraDeveRodar = ligar;
}

// ---------------------------------------------------------
// NOVA FUNÇÃO DE GERENCIAMENTO (Máquina de Estados Sensor B)
// ---------------------------------------------------------
void gerenciarHardwareEsteira() {
    bool sensorA_acionado = (digitalRead(PIN_SENSOR_A) == LOW);
    bool sensorB_leitura  = (digitalRead(PIN_SENSOR_B) == LOW);
    
    // Flag interna para saber se o Sensor B está exigindo parada
    bool bloqueioPeloB = false;

    // --- MÁQUINA DE ESTADOS DO SENSOR B ---
    switch (estadoSensorB) {
        
        // FASE 0: MONITORAMENTO NORMAL
        case B_MONITORANDO:
            if (sensorB_leitura) {
                // Detectou! Inicia bloqueio de 5 segundos
                estadoSensorB = B_BLOQUEIO_5S;
                timerSensorB = millis();
                bloqueioPeloB = true; 
            }
            break;

        // FASE 1: BLOQUEIO DE 5 SEGUNDOS (Esteira parada)
        case B_BLOQUEIO_5S:
            bloqueioPeloB = true; // Força bloqueio independente da leitura atual
            
            // Passaram 5 segundos?
            if (millis() - timerSensorB >= 5000) {
                // Vai para fase de ignorar (libera esteira)
                estadoSensorB = B_IGNORA_3S;
                timerSensorB = millis();
                solicitarEsteira(true);
                bloqueioPeloB = false; 
            }
            break;

        // FASE 2: IGNORAR POR 3 SEGUNDOS (Esteira liberada, cega para Sensor B)
        case B_IGNORA_3S:
            bloqueioPeloB = false; // Não bloqueia (permite rodar)
            
            // Passaram 3 segundos?
            if (millis() - timerSensorB >= 3000) {
                // Volta a monitorar
                estadoSensorB = B_MONITORANDO;
            }
            break;
    }

    // --- DECISÃO FINAL DO HARDWARE ---
    
    // 1. O Sensor A tem prioridade máxima absoluta
    if (sensorA_acionado) {
        digitalWrite(PIN_ESTEIRA, HIGH); // HIGH = PARA
        return;
    }

    // 2. Se Sensor A está livre, verificamos o bloqueio do Sensor B
    if (bloqueioPeloB) {
        digitalWrite(PIN_ESTEIRA, HIGH); // HIGH = PARA
        return;
    }

    // 3. Se ninguém está bloqueando, segue a vontade do software
    // LOW = LIGA, HIGH = DESLIGA
    if (esteiraDeveRodar) {
        digitalWrite(PIN_ESTEIRA, LOW);
    } else {
        digitalWrite(PIN_ESTEIRA, HIGH);
    }
}

void delayComSeguranca(unsigned long ms) {
    unsigned long inicio = millis();
    while (millis() - inicio < ms) {
        gerenciarHardwareEsteira(); 
    }
}
