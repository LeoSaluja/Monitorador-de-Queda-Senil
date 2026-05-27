#include <Arduino.h> 
#include <Wire.h>    

const int MPU_addr = 0x68; 

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 

  Wire.beginTransmission(MPU_addr); 
  Wire.write(0x6B);                 
  Wire.write(0);                    
  Wire.endTransmission(true);       
  
  Serial.println("MPU6050 Inicializado com sucesso!");
  Serial.println("Iniciando leitura dos eixos...");
  delay(1000); 
}

void loop() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);                 
  Wire.endTransmission(false);      

  Wire.requestFrom(MPU_addr, 6, true); 

  int16_t X = (Wire.read() << 8 | Wire.read());
  int16_t Y = (Wire.read() << 8 | Wire.read());
  int16_t Z = (Wire.read() << 8 | Wire.read());

  Serial.printf("Eixo X: %d \t Eixo Y: %d \t Eixo Z: %d\n", X, Y, Z);

  delay(500); 
}
