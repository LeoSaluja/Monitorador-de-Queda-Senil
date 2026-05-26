#include <Arduino.h> // Biblioteca principal do Arduino, necessária para a maioria dos projetos. Fornece funções básicas.
#include <Wire.h> // Biblioteca para comunicação I2C, usada para ler os dados do acelerômetro MPU6050.
#include <math.h> // Biblioteca matemática, necessária para calcular a magnitude da aceleração usando a função sqrt (raiz quadrada).
#include <BLEDevice.h> // Biblioteca para comunicação BLE (Bluetooth Low Energy)
#include <BLEUtils.h> // Biblioteca de utilidades para BLE, usada para manipular UUIDs e outras funções auxiliares.
#include <BLEServer.h> // Biblioteca para criar um servidor BLE, permitindo que o ESP32 ofereça serviços e características para outros dispositivos se conectarem.

// Todas as partes comentadas e marcadas como [FUTURO] são funcionalidades ainda não implementadas, mas previstas para serem colocas no código final.

const int MPU_addr = 0x68; // Endereço I2C do MPU6050. Pode variar, mas 0x68 é o mais comum quando o pino AD0 está conectado ao GND.
const float valor_conversao = 16384.0; // O MPU6050 tem uma escala de 16.384 unidades por G
const int pbuzzer = 18; // Pino para o buzzer piezoeléctrico
const int pbotao = 19; // Pino para o botão de cancelamento
// const int PINO_BATERIA = 34; // Pino para leitura da bateria
const float valor_de_queda = 2.5; // Valor de G que indica uma queda. Pode ser ajustado conforme testes práticos para maior sensibilidade
const unsigned long tempo_falsa_queda = 10000; // 10 segundos em milissegundos
unsigned long tempoInicioAlerta = 0; 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // UUID do serviço BLE que o cinto vai oferecer.
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" // UUID da característica que vai conter o estado do sistema
BLECharacteristic *pCharacteristic;
// Maquina de estados
enum EstadoSistema {
  NORMAL, // Monitorando normalmente, sem quedas detectadas
  ALERTA_LOCAL, // Queda detectada, apitando localmente e aguardando cancelamento manual
  EMERGENCIA_BLE // Tempo esgotado sem cancelamento, sistema incapacitado, enviando alerta via BLE
};
EstadoSistema estadoAtual = NORMAL; //Inicia no estado normal

/* // [FUTURO] FUNÇÃO PARA O ESP32 OUVIR O APLICATIVO
// Classe para dar o Reset remoto pelo aplicativo, caso o cuidador queira resetar o sistema depois de atender a notificação de emergência.
class CallbackDoAplicativo: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      std::string valorRecebido = pChar->getValue();
      if (valorRecebido == "RESET_MEDALHAO") { //Define uma "senha de reset" usada pelo app. 
          estadoAtual = NORMAL;
          pCharacteristic->setValue("0"); // Limpa a gaveta
          Serial.println("Sistema resetado remotamente pelo App.");
      }
      else if (valorRecebido.length() > 0) {
          // Se receber qualquer outra coisa, ignora e avisa no terminal
          Serial.println("[BLE] Tentativa de reset falhou: Comando desconhecido.");
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
  BLEServer *pServer = BLEDevice::createServer(); // Criação do servidor BLE permitindo o ESP a enviar dados para o smartphone
  BLEService *pService = pServer->createService(SERVICE_UUID); // Criação do serviço BLE com o UUID definido, contém os dados que o smartphone pode ler
  pCharacteristic = pService->createCharacteristic( // Criação da característica BLE que vai conter o estado do sistema e ser lida pelo smartphone
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY // |
                      //BLECharacteristic::PROPERTY_WRITE
  );
  /* // [FUTURO] TRECHO PARA PERMITIR A COMUNICAÇÃO ENTRE O ESP E O APLICATIVO
  pCharacteristic->setCallbacks(new CallbackDoAplicativo()); // Ativa a função de ouvir o App
  
  pBateriaCharacteristic = pService->createCharacteristic( //
                             BATTERY_CHAR_UUID,
                             BLECharacteristic::PROPERTY_READ |
                             BLECharacteristic::PROPERTY_NOTIFY
                           );
  */
  pCharacteristic->setValue("0"); // O sistema liga marcando ZERO (Normal)
  pService->start(); // Inicia o serviço BLE
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); // Permite que o celular visualize o ESP32 como um dispositivo BLE para se conectar
  pAdvertising->addServiceUUID(SERVICE_UUID); 
  pAdvertising->setScanResponse(true); // Faz com que o celular veja o nome do dispositivo em vez de apenas "ESP32" ou algo do gênero
  BLEDevice::startAdvertising(); // Começa a anunciar o serviço BLE para que o smartphone possa se conectar
  
  Serial.println("Sistema Iniciado e Bluetooth pronto para conexão. Monitorando...");
}

void loop() {
  // Puxa os dados I2C
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

/*
     [FUTURO] BATERIA
  int leituraADC = analogRead(PINO_BATERIA); // Lê o valor bruto do ADC (0-4095 para 12 bits) do pino conectado à bateria
  
  // Calcula a voltagem real
  // Bateria necessita de 2 resistores físicos para diminuição de tensão de 4.2V para 3.3V. (Por isso a multiplicação por 2.0)
  float voltagem = (leituraADC / 4095.0) * 3.3 * 2.0; 

  //3.2V = 0%, 4.2V = 100%
  int porcentagem = ((voltagem - 3.2) / (4.2 - 3.2)) * 100;

  // Impede porcentagens irreais.
  if (porcentagem > 100) porcentagem = 100;
  if (porcentagem < 0) porcentagem = 0;

  // Fica comentado até você descomentar a gaveta de bateria lá no setup
  char batString[8];
  dtostrf(porcentagem, 1, 0, batString); // Converte o número inteiro para texto
  pBateriaCharacteristic->setValue(batString);
  pBateriaCharacteristic->notify(); // Notifica o smartphone sobre a nova porcentagem de bateria.
*/

  // 4. Maquina de estados para lidar com a lógica de detecção, alerta e comunicação BLE
  switch (estadoAtual) {
    
    case NORMAL:
      // Fica monitorando a magnitude. Se passar do limiar, alerta de provável queda:
      if (magnitude > valor_de_queda) {
        estadoAtual = ALERTA_LOCAL;
        tempoInicioAlerta = millis(); // Marca o exato milissegundo que a queda ocorreu
        digitalWrite(pbuzzer, HIGH); // Liga o apito sonoro
        Serial.println("\n[!] ALERTA! IMPACTO DETECTADO!");
        Serial.println("Iniciando janela de 15 segundos. Aperte o botão para cancelar...");
      }
      break;

    case ALERTA_LOCAL:
      // Fica apitando e lendo o botão. Se o botão for pressionado, cancela o alarme e volta para monitoramento normal:
      if (digitalRead(pbotao) == LOW) {
        estadoAtual = NORMAL; // Falso positivo confirmado, volta pro monitoramento
        digitalWrite(pbuzzer, LOW); // Desliga o apito
        Serial.println("[OK] Falso positivo cancelado pelo usuario. Retornando ao monitoramento.");
        delay(1000); // Pequeno debounce para não ler o botão duas vezes
      }
      // Se o tempo limite se esgotar sem interação manual, assume que o usuário está incapacitado e aciona o protocolo de emergência BLE:
      else if (millis() - tempoInicioAlerta >= tempo_falsa_queda) {
        estadoAtual = EMERGENCIA_BLE;
        digitalWrite(pbuzzer, LOW); // Desliga o apito local
        Serial.println("\n[!!!] TEMPO ESGOTADO. USUARIO INCAPACITADO.");
        Serial.println("Acionando protocolo de emergência...");

        pCharacteristic->setValue("1"); // Atualiza a característica BLE para "1", indicando que o sistema entrou em emergência
        pCharacteristic->notify();      // Notifica os dispositivos conectados (como o smartphone) sobre a mudança de estado
      }
      break;

    case EMERGENCIA_BLE:
      // Estado apenas para manter o esp ligado e conectado, esperando o cuidador atender a notificação no smartphone. 
      //O sistema só volta para NORMAL se o cuidador resetar pelo app.
      break;
  }

  // Apenas imprime os valores no terminal se estiver no estado normal, para não poluir a tela no alarme
  if (estadoAtual == NORMAL) {
    Serial.printf("A: %.2fg\n", magnitude);
  }

  delay(100); 
} 