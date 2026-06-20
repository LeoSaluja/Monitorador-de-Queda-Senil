#include <Arduino.h> // Biblioteca principal do Arduino, necessária para a maioria dos projetos. Fornece funções básicas.
#include <Wire.h> // Biblioteca para comunicação I2C, usada para ler os dados do acelerômetro MPU6050.
#include <math.h> // Biblioteca matemática, necessária para calcular a magnitude da aceleração usando a função sqrt (raiz quadrada).
#include <BLEDevice.h> // Biblioteca para comunicação BLE (Bluetooth Low Energy)
#include <BLEUtils.h> // Biblioteca de utilidades para BLE, usada para manipular UUIDs e outras funções auxiliares.
#include <BLEServer.h> // Biblioteca para criar um servidor BLE, permitindo que o ESP32 ofereça serviços e características.

const int MPU_addr = 0x68; // Endereço I2C do MPU6050.
const float valor_conversao = 16384.0; // O MPU6050 tem uma escala de 16.384 unidades por G
const int pbuzzer = 18; // Pino para o buzzer piezoelétrico
const int pbotao = 19; // Pino para o botão de cancelamento (Falso Positivo)
const float valor_de_queda = 2.5; // Valor de G que indica uma queda. Pode ser ajustado conforme testes práticos.
const unsigned long tempo_falsa_queda = 10000; // Janela de 10 segundos (em milissegundos)
unsigned long tempoInicioAlerta = 0; 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // UUID do serviço BLE.
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" // UUID da característica contendo o estado.
BLECharacteristic *pCharacteristic;

// Maquina de estados
enum EstadoSistema {
  NORMAL, // Monitorando normalmente, sem quedas detectadas
  ALERTA_LOCAL, // Queda detectada, apitando localmente e aguardando cancelamento manual
  EMERGENCIA_BLE // Tempo esgotado sem cancelamento, sistema incapacitado, enviando alerta via BLE
};
EstadoSistema estadoAtual = NORMAL; // Inicia no estado normal

/* // [FUTURO] FUNÇÃO PARA O ESP32 OUVIR O APLICATIVO
// Classe para dar o Reset remoto pelo aplicativo, caso o cuidador queira resetar o sistema depois de atender a emergência.
class CallbackDoAplicativo: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      std::string valorRecebido = pChar->getValue();
      if (valorRecebido == "RESET_MEDALHAO") { 
          estadoAtual = NORMAL;
          pCharacteristic->setValue("0"); // Limpa a gaveta
          Serial.println("Sistema resetado remotamente pelo App.");
      }
    }
  };  
*/

void setup() {
  Serial.begin(115200); // Inicia a comunicação serial para debug
  Wire.begin(21, 22);  // Inicia os pinos I2C (SDA, SCL) para o MPU6050.

  // Configuração dos pinos físicos
  pinMode(pbuzzer, OUTPUT);
  pinMode(pbotao, INPUT_PULLUP); 
  
  // Inicialização do MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);                 
  Wire.write(0);                    
  Wire.endTransmission(true);  

  BLEDevice::init("Detector_De_Queda"); // Nome que vai aparecer no celular
  BLEServer *pServer = BLEDevice::createServer(); 
  BLEService *pService = pServer->createService(SERVICE_UUID); 
  pCharacteristic = pService->createCharacteristic( 
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY 
  );
  
  /* // [FUTURO] TRECHO PARA ATIVAR O RESET PELO APP
  pCharacteristic->setCallbacks(new CallbackDoAplicativo()); 
  */

  pCharacteristic->setValue("0"); // O sistema liga marcando ZERO (Normal)
  pService->start(); 
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); 
  pAdvertising->addServiceUUID(SERVICE_UUID); 
  pAdvertising->setScanResponse(true); 
  BLEDevice::startAdvertising(); 
  
  Serial.println("Sistema Iniciado e Bluetooth pronto para conexão. Monitorando...");
}

void loop() {
  // Puxa os dados I2C do MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);                 
  Wire.endTransmission(false);        
  Wire.requestFrom(MPU_addr, 6, true); 

 // 1. Lê os valores brutos dos eixos X, Y e Z do acelerômetro
  int16_t X = (Wire.read() << 8 | Wire.read());
  int16_t Y = (Wire.read() << 8 | Wire.read());
  int16_t Z = (Wire.read() << 8 | Wire.read());

  // 2. Dividimos para achar a força G
  float gX = X / valor_conversao;
  float gY = Y / valor_conversao;
  float gZ = Z / valor_conversao;
  
  // 3. Calculamos a magnitude total da aceleração pelo teorema de pitágoras
  float magnitude = sqrt((gX * gX) + (gY * gY) + (gZ * gZ));

  // 4. Maquina de estados principal
  switch (estadoAtual) {
    
    case NORMAL:
      // Fica monitorando a magnitude. Se passar do limiar, alerta de provável queda:
      if (magnitude > valor_de_queda) {
        estadoAtual = ALERTA_LOCAL;
        tempoInicioAlerta = millis(); // Marca o exato milissegundo que a queda ocorreu
        digitalWrite(pbuzzer, HIGH); // Liga o apito sonoro
        Serial.println("\n[!] ALERTA! IMPACTO DETECTADO!");
        Serial.println("Iniciando janela de 10 segundos. Aperte o botão para cancelar...");
      }
      break;

    case ALERTA_LOCAL:
      // Fica apitando e lendo o botão. Se o botão for pressionado, cancela o alarme e volta:
      if (digitalRead(pbotao) == LOW) {
        estadoAtual = NORMAL; // Falso positivo confirmado
        digitalWrite(pbuzzer, LOW); // Desliga o apito
        Serial.println("[OK] Falso positivo cancelado pelo usuario. Retornando ao monitoramento.");
        delay(1000); // Pequeno debounce para não ler o botão duas vezes
      }
      // Se o tempo limite se esgotar sem interação, assume incapacitação e aciona BLE:
      else if (millis() - tempoInicioAlerta >= tempo_falsa_queda) {
        estadoAtual = EMERGENCIA_BLE;
        digitalWrite(pbuzzer, LOW); // Desliga o apito local
        Serial.println("\n[!!!] TEMPO ESGOTADO. USUARIO INCAPACITADO.");
        Serial.println("Acionando protocolo de emergência...");

        pCharacteristic->setValue("1"); // Atualiza o BLE para "1" (Emergência)
        pCharacteristic->notify();      // Dispara a notificação para o smartphone
      }
      break;

    case EMERGENCIA_BLE:
      // Estado para manter conectado. Só volta para NORMAL se o cuidador resetar pelo app (se implementado)
      // ou se o botão físico for adaptado futuramente para fazer o reset geral.
      break;
  }

  // Apenas imprime os valores se estiver normal, para não poluir a tela no alarme
  if (estadoAtual == NORMAL) {
    Serial.printf("A: %.2fg\n", magnitude);
  }

  delay(100); 
}
