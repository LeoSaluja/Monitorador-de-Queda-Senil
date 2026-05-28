# Monitorador-de-Queda-Senil
Repositório para o trabalho de Projeto Integrador 2 do curso de Engenharia de Computação / UFSM.

Alunos: Leonardo Sodré dos Santos - 202420389;
        Augusto V. Piovesan - 202420066;;
        Alex D. Manfio - 2024520012;
        Lucas P. Togni - 202420907;

Este é um projeto de hardware e software desenvolvido com fins didáticos por alunos da UFSM, com o objetivo de monitorar e garantir a segurança de pessoas idosas.

O dispositivo utiliza um ESP32, bateria e um sensor acelerômetro/giroscópio para identificar quedas. Ao detectar um movimento brusco suspeito, um speaker emite um bipe de alerta. Para evitar falsos positivos, o usuário pode apertar um botão físico para cancelar o alarme. Caso o botão não seja pressionado, o dispositivo se conecta a um aplicativo no celular do idoso, que envia a ocorrência para um servidor, notificando imediatamente cuidadores e familiares sobre a emergência.
