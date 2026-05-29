# Monitorador-de-Queda-Senil

Repositório para o trabalho de Projeto Integrador II do curso de Engenharia de Computação / UFSM.

## Integrantes
* **Leonardo Sodré dos Santos** - Matrícula: 202420389
* **Augusto V. Piovesan** - Matrícula: 202420066
* **Alex D. Manfio** - Matrícula: 2024520012
* **Lucas P. Togni** - Matrícula: 202420907

## Visão Geral
Este é um projeto de hardware e software desenvolvido com fins didáticos por alunos da UFSM, com o objetivo de monitorar e garantir a segurança de pessoas idosas.

O dispositivo utiliza um ESP32, uma bateria LiPo e um sensor acelerômetro/giroscópio para identificar quedas em tempo real. Ao detectar um impacto ou movimento brusco suspeito, um buzzer emite um sinal sonoro de alerta local. Para evitar o disparo de falsos positivos, o usuário dispõe de uma janela de tempo para pressionar um botão físico de cancelamento. Caso o alarme não seja interrompido manualmente, o dispositivo estabelece uma conexão via Bluetooth Low Energy (BLE) com um aplicativo no smartphone do idoso. A aplicação móvel obtém a geolocalização atual e envia a ocorrência para um servidor, notificando imediatamente os cuidadores e familiares cadastrados sobre a situação de emergência.

## Organização das Pastas do Projeto / Repositório Git

* **`README.md`**: Arquivo de texto principal com a introdução do projeto, contextualização do escopo e identificação dos integrantes da equipe.
* **`/Hardware`**: Diretório centralizado no Hardware, contendo toda a lógica de programação inseridada no ESP32 para controlar demais chips. Possui as seguintes subpastas:
  * **`/include` e `/lib`**: Diretórios reservados para o mapeamento de cabeçalhos internos e gerenciamento de bibliotecas externas adicionadas ao projeto.
  * **`/src/main.cpp`**: Código-fonte principal que implementa a arquitetura de máquina de estados, interrupções dos botões e a lógica de processamento dos dados do sensor.
  * **`README.md`**: Documento de texto dedicado a explicar a arquitetura lógica e o fluxo de execução do firmware embarcado.
  * **`platformio.ini`**: Arquivo de configuração global do ambiente de desenvolvimento, responsável pelas definições da placa, velocidade de comunicação serial e dependências de compilação.
  * **`teste.cpp`**: **Código inicial unicamente utilizado para teste e validação individual dos componentes físicos (sensores e atuadores).**
* **`/App`**: Diretório destinado aos arquivos de código e módulos do aplicativo para dispositivo móvel.
