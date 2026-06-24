#include <Arduino.h> // Biblioteca principal do Arduino.
#include <Wire.h> // Biblioteca para comunicação I2C (MPU6050).
#include <math.h> // Biblioteca matemática (teorema de pitágoras).
#include <BLEDevice.h> // Biblioteca para comunicação BLE.
#include <BLEUtils.h> // Utilitários BLE.
#include <BLEServer.h> // Criação do servidor BLE.

const int MPU_addr = 0x68; // Endereço I2C do MPU6050.
const float valor_conversao = 16384.0; // Escala do MPU6050 (16.384 unidades por G).
const int pbuzzer = 18; // Pino para o buzzer piezoelétrico.
const int pbotao = 26; // Pino para o botão de cancelamento (Falso Positivo).
const float valor_de_queda = 2.15; // Valor de G que indica uma queda.
const unsigned long tempo_falsa_queda = 7000; // Janela de 7 segundos (em milissegundos).
unsigned long tempoInicioAlerta = 0; 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b" 
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" 
BLECharacteristic *pCharacteristic;

// Maquina de estados principal
enum EstadoSistema {
  NORMAL, // Monitorando normalmente
  ALERTA_LOCAL, // Queda detectada, apitando localmente
  EMERGENCIA_BLE // Tempo esgotado, enviando alerta via BLE
};
EstadoSistema estadoAtual = NORMAL; 

// CLASSE DESCOMENTADA: Permite que o ESP32 ouça o aplicativo
class CallbackDoAplicativo: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      std::string valorRecebido = pChar->getValue();
      
      // Se o aplicativo enviar a senha correta, o sistema sai da emergência e volta a monitorar
      if (valorRecebido == "RESET_MEDALHAO") { 
          estadoAtual = NORMAL;
          pCharacteristic->setValue("0"); // Limpa a gaveta BLE
          Serial.println("Sistema resetado remotamente pelo App. Voltando ao monitoramento.");
      }
      else if (valorRecebido.length() > 0) {
          Serial.println("[BLE] Tentativa de reset falhou: Comando desconhecido.");
      }
    }
};  

void setup() {
  Serial.begin(115200); 
  Wire.begin(21, 22);  

  pinMode(pbuzzer, OUTPUT);
  pinMode(pbotao, INPUT_PULLUP); 
  
  // Inicialização do MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);                 
  Wire.write(0);                    
  Wire.endTransmission(true);  

  BLEDevice::init("Detector_De_Queda"); 
  BLEServer *pServer = BLEDevice::createServer(); 
  BLEService *pService = pServer->createService(SERVICE_UUID); 
  
  // A propriedade WRITE foi adicionada para o celular conseguir enviar o comando de reset
  pCharacteristic = pService->createCharacteristic( 
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE 
  );
  
  // ATIVADO: Vincula a função de ouvir o App à característica BLE
  pCharacteristic->setCallbacks(new CallbackDoAplicativo()); 

  pCharacteristic->setValue("0"); 
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

  // 1. Lê os valores brutos dos eixos X, Y e Z
  int16_t X = (Wire.read() << 8 | Wire.read());
  int16_t Y = (Wire.read() << 8 | Wire.read());
  int16_t Z = (Wire.read() << 8 | Wire.read());

  // 2. Dividimos para achar a força G individual
  float gX = X / valor_conversao;
  float gY = Y / valor_conversao;
  float gZ = Z / valor_conversao;
  
  // 3. Calculamos a magnitude total da aceleração pelo teorema de pitágoras
  float magnitude = sqrt((gX * gX) + (gY * gY) + (gZ * gZ));

  // 4. Maquina de estados principal
  switch (estadoAtual) {
    
    case NORMAL:
      // Fica monitorando a magnitude. Se passar do limiar, vai para o alerta:
      if (magnitude > valor_de_queda) {
        estadoAtual = ALERTA_LOCAL;
        tempoInicioAlerta = millis(); 
        digitalWrite(pbuzzer, HIGH); 
        Serial.println("\n[!] ALERTA! IMPACTO DETECTADO!");
        Serial.println("Iniciando janela de 10 segundos. Aperte o botão para cancelar...");
      }
      break;

    case ALERTA_LOCAL:
      // Se o botão for pressionado fisicamente, cancela o alarme:
      if (digitalRead(pbotao) == LOW) {
        estadoAtual = NORMAL; 
        digitalWrite(pbuzzer, LOW); 
        Serial.println("[OK] Falso positivo cancelado pelo usuario. Retornando ao monitoramento.");
        delay(1000); // Debounce
      }
      // Se o tempo limite se esgotar sem interação, aciona a emergência BLE:
      else if (millis() - tempoInicioAlerta >= tempo_falsa_queda) {
        estadoAtual = EMERGENCIA_BLE;
        digitalWrite(pbuzzer, LOW); // Silencia o apito local
        Serial.println("\n[!!!] TEMPO ESGOTADO. USUARIO INCAPACITADO.");
        Serial.println("Acionando protocolo de emergência via Bluetooth...");

        pCharacteristic->setValue("1"); // Muda o estado para emergência
        pCharacteristic->notify();      // Notifica o smartphone Android
      }
      break;

    case EMERGENCIA_BLE:
      // O sistema fica travado aqui. Só volta ao normal se o código da classe 'CallbackDoAplicativo'
      // receber a string "RESET_MEDALHAO" vinda do celular.
      break;
  }

  if (estadoAtual == NORMAL) {
    Serial.printf("A: %.2fg\n", magnitude);
  }

  delay(100); 
}
