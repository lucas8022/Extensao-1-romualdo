#include <Arduino.h>
#include <ServoEasing.hpp> 
#include "BracoIndustrial.h"

// --- DEFINIÇÃO DOS PINOS (I/O) ---
const int PIN_SENSOR_A = 26; // Sensor da Peça (LOW = Objeto detectado)
const int PIN_SENSOR_B = 24; // Sensor do segundo braço 
const int PIN_ESTEIRA  = 22; // Saída: Controle do motor
const int PIN_START    = 8; // Botão Verde
const int PIN_STOP     = 9; // Botão Vermelho

// --- INSTANCIAÇÃO DO BRAÇO ---
BracoIndustrial robot(5, 6, 7, 11, 10);    // adicionando os pinos aos servos
//RobotArm equipe2(15,16,17,18,19);

// --- ESTADOS DO SISTEMA (FSM) ---
enum EstadoSistema {
    EM_ESPERA_START, 
    INICIANDO,       
    AGUARDANDO_PECA, 
    EXECUTANDO_CICLO,
    PARANDO,         
    REPOUSO_HOME     
};

EstadoSistema estadoAtual = EM_ESPERA_START;

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
void delayComSeguranca(unsigned long ms); // Substitui o delay() comum

// Configuracao inicial do sistema
void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_SENSOR_A, INPUT_PULLUP);
    pinMode(PIN_SENSOR_B, INPUT_PULLUP); 
    pinMode(PIN_ESTEIRA, OUTPUT);
    pinMode(PIN_START, INPUT_PULLUP);
    pinMode(PIN_STOP, INPUT_PULLUP);
    
    robot.setZeroMaquina(30, 120, 0, 90, 50);  //GARRA 50 -> 180
    robot.setZeroPeca(150, 85, 60, 97, 75);     
    robot.setSpeed(30); 

    Serial.println(F("Sistema Ligado."));
    robot.begin();
    delayComSeguranca(100);

    Serial.println(F("Indo para Zero Maquina (Home)..."));
    robot.goHome();
    solicitarEsteira(false); 
    
    // Enquanto move, checa o sensor B para cortar a esteira se necessário
    while(robot.isMoving()) { 
        gerenciarHardwareEsteira(); 
        delay(10); 
    }

    Serial.println(F("PRONTO. Aguardando botao START."));
    estadoAtual = EM_ESPERA_START;
}

// Loop principal: maquina de estados do ciclo
void loop() {
    lerBotoes(); 
    
    gerenciarHardwareEsteira();     // Atualiza saída da esteira com prioridade de segurança

    switch (estadoAtual) {
        
        case EM_ESPERA_START:
            break;

        case INICIANDO:
            if (!robot.isMoving()) {
                Serial.println(F("Posicionado. Solicitando esteira..."));
                solicitarEsteira(true); 
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

            // Sensor A (Peça Chegou)
            if (digitalRead(PIN_SENSOR_A) == LOW) {
                Serial.println(F("Sensor A: Objeto Detectado! Parando esteira."));
                solicitarEsteira(false); 
                delay(130);
                //delayComSeguranca(3000); 
                faseCiclo = 0;
                estadoAtual = EXECUTANDO_CICLO;
            }
            // Timeout -> Home (mantendo a intenção de esteira ligada)
            else if (millis() - tempoUltimaAtividade > TEMPO_TIMEOUT) {
                Serial.println(F("Ocioso. Indo para Home (Esteira continua ON)..."));
                // solicitarEsteira(false); // <--- REMOVIDO PARA MANTER LIGADA
                robot.move3(0);
                delayComSeguranca(2000); 
                robot.goHome();          
                estadoAtual = REPOUSO_HOME;
            }
            break;

        case REPOUSO_HOME:
            if (robot.isMoving())
            {
                gerenciarHardwareEsteira();
                return;
            } 

            // Se detectar objeto no Sensor A enquanto repousa
            if (digitalRead(PIN_SENSOR_A) == LOW) {
                Serial.println(F("Objeto visto (Sensor A)! Parando esteira e indo pegar objeto..."));
                
                solicitarEsteira(false); // Para a esteira imediatamen
                
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

// Leitura dos botoes e transicoes de estado
void lerBotoes() {
    if (digitalRead(PIN_START) == LOW) {
        if (estadoAtual == EM_ESPERA_START) {
            Serial.println(F("START pressionado!"));
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

// Sequencia do ciclo de pega e deposita
void executarCicloPassoAPasso() {
    if (robot.isMoving()) 
    {
        gerenciarHardwareEsteira();
        return;
    }

    if (emPausaDelay) {
        if (millis() - tempoInicioDelay >= duracaoDelay) {
            emPausaDelay = false;
        } else {
            // Continua checando segurança enquanto espera o tempo passar
            gerenciarHardwareEsteira();
            return;
        }
    }

    RobotPose deposito = {75, 130, 31, 75, 110};
    RobotPose ajuste    = {75, 130, 10, 97, 50}; 

    // (150, 85, 70, 105, 50)     VALORES ZERO PECA (referência)

    switch (faseCiclo) {
        case 0:
            Serial.println(F("CASE 0: BAIXANDO BRAÇO"));
            robot.move3(75);
            //iniciarDelay(800); 
            faseCiclo++;
            break;
        case 1: 
            Serial.println(F("CASE 1: FECHANDO GARRA"));
            robot.moveGarra(114); // 130
            //iniciarDelay(800); 
            faseCiclo++;
            break;

        case 2:
            Serial.println(F("CASE 2: SUBINDO BRAÇO"));
            robot.move3(5);
            //delayComSeguranca(1000);
            //iniciarDelay(800); 
            faseCiclo++;
            break;

        case 3: // Leva para depósito
            Serial.println(F("CASE 3: DEPÓSITO"));
            solicitarEsteira(true); // O robô pede para ligar
            robot.moveToPose(deposito);
            faseCiclo++;
            break;

        case 4: // Solta
            Serial.println(F("CASE 4: ABRINDO GARRA"));
            robot.moveGarra(50);
            //iniciarDelay(800);
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

// Agenda um delay nao bloqueante
void iniciarDelay(unsigned long ms) {
    emPausaDelay = true;
    tempoInicioDelay = millis();
    duracaoDelay = ms;
}

// --- CONTROLE INTELIGENTE DA ESTEIRA ---

// Define apenas a intenção do robô
void solicitarEsteira(bool ligar) {
    esteiraDeveRodar = ligar;
}

//  Controla o pino real, com prioridade para o Sensor B
// void gerenciarHardwareEsteira() {
//     // 1. PRIORIDADE TOTAL: Sensor B (Segurança)
//     // Se o sensor B detectar algo (HIGH), CORTA a energia da esteira
//     if (digitalRead(PIN_SENSOR_B) == LOW || digitalRead(PIN_SENSOR_B) == LOW) {
//         delayComSeguranca(150);
//         digitalWrite(PIN_ESTEIRA, LOW); 
//     } 
//     // 2. Se o Sensor B estiver livre, obedece ao robô
//     else {

//         if (esteiraDeveRodar) {
//             digitalWrite(PIN_ESTEIRA, LOW);
//         } else {
//             digitalWrite(PIN_ESTEIRA, HIGH);
//         }
//     }
// }

// Controla o pino real, com prioridade para o Sensor B
void gerenciarHardwareEsteira() {
    bool sensorB_acionado = (digitalRead(PIN_SENSOR_B) == LOW);
    bool sensorA_acionado = (digitalRead(PIN_SENSOR_A) == LOW);

    // Prioridade total: qualquer sensor ativo corta a esteira
    if (sensorB_acionado || sensorA_acionado) {
        
        digitalWrite(PIN_ESTEIRA, HIGH);
        return;
    }

    digitalWrite(PIN_ESTEIRA, esteiraDeveRodar ? LOW : HIGH);
}


// Substituto do delay: espera o tempo passar, mas vigia o sensor B
void delayComSeguranca(unsigned long ms) {
    unsigned long inicio = millis();
    while (millis() - inicio < ms) {
        // A cada milissegundo de espera, checa se precisa parar a esteira
        gerenciarHardwareEsteira(); 
    }
}