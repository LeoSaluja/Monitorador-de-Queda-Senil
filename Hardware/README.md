# Detector de Queda (Módulo Hardware)

Este repositório contém o código-fonte e a documentação de hardware do projeto "Detector de Queda Seníl", um dispositivo vestível (wearable) de segurança focado na detecção de quedas para usuários idosos. 

O sistema monitora a aceleração do usuário em tempo real. Ao detectar um impacto (Força G elevada), ele emite um alerta sonoro local. Caso o usuário não cancele o alerta (indicando uma possível incapacitação), o dispositivo aciona um protocolo de emergência via Bluetooth Low Energy (BLE) para um aplicativo de smartphone.

OBS: O código "teste.cpp" foi o código utilizado para teste inicial apenas para saber se os componentes vieram com defeito ou não. O código final é o "main.cpp" dentro da pasta src
---

## Esquemático simples dos pinos para melhor compreendimento do código:

| Componente | Pino Físico | Pino ESP32 | Função |
| :--- | :--- | :--- | :--- |
| **MPU6050** | SDA | `GPIO 21` | Comunicação de Dados I2C |
| **MPU6050** | SCL | `GPIO 22` | Clock I2C |
| **Buzzer** | Positivo | `GPIO 18` | Alarme sonoro local |
| **Botão** | Perna 1 | `GPIO 19` | Cancelamento (INPUT_PULLUP) |
| **Botão** | Perna 2 | `GND` | Referência |
| **TP4056** | OUT+ | `VIN` / `5V` | Alimentação via Chave Deslizante |
| **TP4056** | OUT- | `GND` | Terra comum do sistema |

> **Nota de Montagem:** A chave deslizante (Liga/Desliga) interrompe a trilha entre o `OUT+` do TP4056 e o `VIN` do ESP32. A bateria permanece soldada nos pinos `B+` e `B-` do módulo, permitindo o carregamento mesmo com o cinto desligado.

---

## Sobre o BLE (Bluetooth Low Energy)

O ESP32 opera como um Servidor BLE, aguardando a conexão do smartphone do cuidador. Assim será passado as informações de estados e dados necessários entre o aplicativo e o hardware.

* **Nome do Dispositivo:** `Detector_De_Queda`
* **Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
* **Characteristic UUID (Estado):** `beb5483e-36e1-4688-b7f5-ea07361b26a8`

--

##Máquina de Estados
1. `0` **(NORMAL):** Sistema monitorando. Nenhuma queda detectada.
2. `ALERTA LOCAL:` Impacto > 2.5G detectado. O buzzer apita por 10 segundos aguardando o cancelamento manual pelo botão.
   OBS: o tempo de 10 segs é temporário para testes, pode ser mudado futuramente.
3. `1` **(EMERGÊNCIA BLE):** Tempo limite esgotado sem cancelamento. O sistema envia "1" via BLE para acionar o socorro no aplicativo.

---

## Implementações futuras:

Este código reflete a versão semi-pronta do trabalho, quando o aplicativo tiver pronto iremos adicionar as seguintes partes (que já estão comentadas no código):

- [ ] **Leitura de Bateria:** Implementar divisor de tensão no pino `34` e ativar a característica BLE dedicada para envio da porcentagem da bateria ao app.
- [ ] **Reset Remoto:** Habilitar a permissão `PROPERTY_WRITE` no BLE para permitir que o cuidador reinicie o estado de emergência do cinto diretamente pelo smartphone.

---
*Desenvolvido em C++ utilizando o framework Arduino e PlatformIO.*
