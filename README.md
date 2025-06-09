# 🚨 Projeto Drone de Emergência IoT com ESP32

Este projeto demonstra uma solução IoT embarcada utilizando o microcontrolador **ESP32** para monitoramento de emergências ambientais, como **enchentes**, **incêndios** e **deslizamentos**. Quando um evento crítico é detectado, o sistema simula o envio de um drone com alertas visuais e sonoros, e publica dados via **MQTT** para um painel remoto ou backend.

---

## 📋 Funcionalidades

- Leitura de sensores analógicos (água, calor, solo)
- Detecção automática de eventos críticos com lógica de confirmação
- Emissão de sinal sonoro e ativação de LED indicando missão
- Publicação de dados para tópicos MQTT em tempo real
- Normalização automática e cancelamento da missão
- Comandos MQTT para ativar/desativar o drone manualmente

---

## 🔌 Hardware Utilizado

| Componente            | Descrição                    |
|----------------------|------------------------------|
| ESP32 (DevKit)       | Microcontrolador principal   |
| Sensor de Água       | Simulado com potenciômetro   |
| Sensor de Incêndio   | Simulado com LDR             |
| Sensor de Solo       | Simulado com potenciômetro   |
| LED                  | Representa o drone em missão |
| Buzzer               | Alerta sonoro do drone       |

---

## 🌐 Conexão Wi-Fi e MQTT

- **SSID**: `Wokwi-GUEST` (simulado no Wokwi)
- **Broker MQTT**: `broker.hivemq.com`
- **Porta**: `1883`

---

## 📡 Tópicos MQTT

| Tópico                      | Descrição                                |
|----------------------------|------------------------------------------|
| `/sensor/agua`             | Publica valor analógico do sensor água   |
| `/sensor/incendio`         | Publica valor analógico do sensor fogo   |
| `/sensor/deslizamento`     | Publica valor analógico do sensor solo   |
| `/drone/status`            | Estado atual do drone                    |
| `/alerta/confirmado`       | Emergência confirmada                    |
| `/alerta/cancelado`        | Emergência encerrada                     |
| `/drone/controle`          | Comando externo (`ativar`, `desativar`)  |

---

## 🧠 Lógica de Funcionamento

1. **Leitura contínua** dos sensores.
2. Se algum sensor ultrapassar um limite de emergência, o sistema:
   - Aguarda tempo de confirmação (10s);
   - Valida se o evento persiste;
   - **Ativa o drone** com LED e buzzer.
3. O sistema continua monitorando até que:
   - O sensor volte ao normal **por tempo mínimo de missão (15s)**;
   - Ou o controle externo envie um comando de "desativar".

---

## ▶️ Como Usar

1. **Instale as bibliotecas necessárias** no Arduino IDE:
   - `WiFi.h`
   - `PubSubClient.h`

2. **Configure seu Wi-Fi** (caso use fora do Wokwi):
   ```cpp
   const char* ssid = "SEU_WIFI";
   const char* password = "SENHA";
