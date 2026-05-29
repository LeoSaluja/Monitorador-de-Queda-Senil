# Detector de queda (módulo aplicativo móvel)

Este repositório contém o código-fonte do aplicativo Android desenvolvido em Java para o projeto integrador "Detector de Queda Senil". O aplicativo atua como a central de comunicação e socorro, trabalhando em conjunto com o dispositivo vestível (*wearable* baseado em ESP32) utilizado pelo idoso.

O sistema móvel é projetado para operar em segundo plano. Ao receber o sinal de alerta de emergência do cinto (via *Bluetooth Low Energy*), o aplicativo captura imediatamente as coordenadas GPS exatas do smartphone, realiza a geocodificação reversa para obter o endereço em formato de texto e transmite um log de emergência via requisição HTTP POST para um servidor ou máquina virtual remota.

---

## Funcionalidades principais

* **Monitoramento BLE (*Bluetooth Low Energy*):** escuta ativa do servidor BLE hospedado no ESP32 para identificar mudanças de estado (normal -> emergência).
* **Captura de localização de alta precisão:** utiliza a API `FusedLocationProviderClient` do *Google Play Services* para obter as coordenadas de latitude e longitude de forma rápida e precisa.
* **Geocodificação reversa (*Geocoder*):** converte as coordenadas brutas em um endereço humano legível (rua, número, bairro, CEP e cidade) para facilitar o resgate.
* **Transmissão de dados (nuvem/servidor):** envio automatizado do pacote de emergência (dados da queda + localização) para uma API ou *webhook* via requisição HTTP POST assíncrona.

---

## Especificações de comunicação (BLE)

Para garantir a integração ponta a ponta com o hardware, o aplicativo é configurado para parear e ler os dados das seguintes UUIDs expostas pelo cinto:

* **Nome do dispositivo:** `Detector_De_Queda`
* **Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
* **Characteristic UUID (estado):** `beb5483e-36e1-4688-b7f5-ea07361b26a8`

> **Lógica de gatilho:** quando o valor da *Characteristic* (estado) for alterado para `1` (indicando que o tempo limite de cancelamento físico do *buzzer* se esgotou sem resposta do usuário), o aplicativo executa os métodos de captura de GPS e disparo de rede.

---

## Permissões do Android (AndroidManifest.xml)

Para que o aplicativo funcione corretamente, as seguintes permissões são exigidas pelo sistema operacional e solicitadas ao usuário na primeira execução:

| Permissão | Função no projeto |
| :--- | :--- |
| `ACCESS_FINE_LOCATION` | Acesso ao GPS de alta precisão do celular. |
| `ACCESS_COARSE_LOCATION` | Acesso à localização aproximada via torres de celular ou Wi-Fi. |
| `INTERNET` | Necessário para envio do log POST para o servidor. |
| `BLUETOOTH_SCAN` / `CONNECT` | (Android 12+) necessário para buscar e conectar ao ESP32. |
| `FOREGROUND_SERVICE` | Permite que o app rode de forma ininterrupta em segundo plano. |

> **Nota de rede:** o aplicativo conta com a diretiva `android:usesCleartextTraffic="true"`, permitindo o envio de dados via HTTP padrão durante a fase de testes em laboratório ou máquinas virtuais sem certificados SSL.

---

## Próximos passos (implementações futuras)

Este código reflete a versão base de captura de localização e envio via rede. As próximas atualizações incluirão:

- [ ] **Integração BLE nativa:** implementar as classes `BluetoothGatt` no Java para estabelecer a conexão direta e leitura automática da máquina de estados do ESP32.
- [ ] **Serviço em segundo plano constante (*Foreground Service*):** migrar o ciclo de vida do aplicativo para um *Service* atrelado a uma notificação fixa, garantindo que o Android não encerre o processo de monitoramento para economizar bateria.
- [ ] **Leitura de bateria do dispositivo:** adicionar a leitura de uma nova característica BLE para mostrar o nível de bateria do cinto na tela do celular.
- [ ] **Reset remoto (escrita BLE):** adicionar um botão no aplicativo capaz de enviar um sinal de `PROPERTY_WRITE` de volta para o ESP32, permitindo que o cuidador cancele o estado de alarme remotamente.

---
*Desenvolvido em Java utilizando o Android Studio.*
