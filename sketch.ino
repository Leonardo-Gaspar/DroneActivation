#include <WiFi.h>
#include <PubSubClient.h>

// ================= PROTÓTIPOS DE FUNÇÃO =================
void desativarDrone(bool normalizacao = false);

// ================= CONFIGURAÇÕES =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Pinos (conforme diagram.json)
#define SENSOR_AGUA 34
#define SENSOR_INCENDIO 35
#define SENSOR_DESLIZAMENTO 32
#define LED_DRONE 25
#define BUZZER 26

// Limiares
const int LIMIAR_EMERGENCIA = 2500;
const int LIMIAR_CONFIRMACAO_INCENDIO = 3250; // 30% acima do normal
const int TEMPO_CONFIRMACAO = 10000; // 10 segundos
const unsigned long DURACAO_MISSAO = 30000; // 30 segundos

// Variáveis de estado
bool emMissao = false;
bool emergenciaAtiva = false;
String tipoEmergencia = "";
unsigned long tempoMissao = 0;
unsigned long ultimoAlertaAgua = 0;
unsigned long ultimoAlertaIncendio = 0;
unsigned long ultimoAlertaDeslizamento = 0;
const unsigned long TEMPO_MINIMO_MISSAO = 15000; // 15s mínimo de atuação

WiFiClient espClient;
PubSubClient client(espClient);

// ================= SETUP =================
void setup() {
  pinMode(LED_DRONE, OUTPUT);
  digitalWrite(LED_DRONE, LOW);
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);
  
  Serial.begin(115200);
  Serial.println("\nIniciando sistema Drone de Emergência IoT...");
  Serial.println("Verificando sensores...");

  delay(2000); // Estabilização

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("Valores iniciais:");
  Serial.print("Água: "); Serial.println(analogRead(SENSOR_AGUA));
  Serial.print("Incêndio: "); Serial.println(analogRead(SENSOR_INCENDIO));
  Serial.print("Deslizamento: "); Serial.println(analogRead(SENSOR_DESLIZAMENTO));
}

// ================= LOOP PRINCIPAL =================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  static unsigned long ultimaLeitura = 0;
  if (millis() - ultimaLeitura > 2000) {
    verificarSensores();
    monitorarNormalizacao();
    ultimaLeitura = millis();

    Serial.print("Status: ");
    Serial.println(emMissao ? "EM MISSÃO (" + tipoEmergencia + ")" : "Em espera");
  }

  if (emMissao) {
    tone(BUZZER, 1000, 500); // Sirene intermitente
  }
}

// ================= FUNÇÕES DE CONEXÃO =================
void setup_wifi() {
  delay(10);
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    if (client.connect("ESP32DroneClient")) {
      Serial.println("conectado");
      client.subscribe("/drone/controle");
      client.publish("/drone/status", "reconectado");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando em 5s...");
      delay(5000);
    }
  }
}

// ================= LÓGICA DE DETECÇÃO INTELIGENTE =================
void verificarSensores() {
  int agua = analogRead(SENSOR_AGUA);
  int incendio = analogRead(SENSOR_INCENDIO);
  int deslizamento = analogRead(SENSOR_DESLIZAMENTO);

  client.publish("/sensor/agua", String(agua).c_str());
  client.publish("/sensor/incendio", String(incendio).c_str());
  client.publish("/sensor/deslizamento", String(deslizamento).c_str());

  Serial.print("Leituras [Água/Incêndio/Deslizamento]: ");
  Serial.print(agua); Serial.print("/");
  Serial.print(incendio); Serial.print("/");
  Serial.println(deslizamento);

  if (agua > LIMIAR_EMERGENCIA && !emergenciaAtiva) {
    if (millis() - ultimoAlertaAgua > TEMPO_CONFIRMACAO) {
      if (deslizamento < LIMIAR_EMERGENCIA * 0.7) {
        ativarDrone("enchente");
      }
    } else if (ultimoAlertaAgua == 0) {
      ultimoAlertaAgua = millis();
      Serial.println("Alerta de água detectado - Aguardando confirmação...");
    }
  } else {
    ultimoAlertaAgua = 0;
  }

  if (incendio > LIMIAR_EMERGENCIA && !emergenciaAtiva) {
    if (millis() - ultimoAlertaIncendio > TEMPO_CONFIRMACAO) {
      if (incendio > LIMIAR_CONFIRMACAO_INCENDIO) {
        ativarDrone("incendio");
      }
    } else if (ultimoAlertaIncendio == 0) {
      ultimoAlertaIncendio = millis();
      Serial.println("Alerta de incêndio detectado - Aguardando confirmação...");
    }
  } else {
    ultimoAlertaIncendio = 0;
  }

  if (deslizamento > LIMIAR_EMERGENCIA && !emergenciaAtiva) {
    if (millis() - ultimoAlertaDeslizamento > TEMPO_CONFIRMACAO) {
      if (agua < LIMIAR_EMERGENCIA * 0.5) {
        ativarDrone("deslizamento");
      }
    } else if (ultimoAlertaDeslizamento == 0) {
      ultimoAlertaDeslizamento = millis();
      Serial.println("Alerta de deslizamento detectado - Aguardando confirmação...");
    }
  } else {
    ultimoAlertaDeslizamento = 0;
  }
}

// ================= MONITORAMENTO DE NORMALIZAÇÃO =================
void monitorarNormalizacao() {
  if (!emMissao) return;

  int agua = analogRead(SENSOR_AGUA);
  int incendio = analogRead(SENSOR_INCENDIO);
  int deslizamento = analogRead(SENSOR_DESLIZAMENTO);
  bool normalizou = false;

  if (tipoEmergencia == "enchente" && agua < LIMIAR_EMERGENCIA * 0.8) {
    Serial.println("Nível de água normalizado");
    normalizou = true;
  } 
  else if (tipoEmergencia == "incendio" && incendio < LIMIAR_CONFIRMACAO_INCENDIO * 0.7) {
    Serial.println("Temperatura normalizada");
    normalizou = true;
  }
  else if (tipoEmergencia == "deslizamento" && deslizamento < LIMIAR_EMERGENCIA * 0.6) {
    Serial.println("Terreno estabilizado");
    normalizou = true;
  }

  if (normalizou && (millis() - tempoMissao > TEMPO_MINIMO_MISSAO)) {
    desativarDrone(true); // Cancela por normalização
  }
}

// ================= FUNÇÕES DE ATUAÇÃO =================
void ativarDrone(String tipoAlerta) {
  emMissao = true;
  emergenciaAtiva = true;
  tipoEmergencia = tipoAlerta;
  tempoMissao = millis();
  digitalWrite(LED_DRONE, HIGH);

  String mensagemStatus = "em_missao_" + tipoAlerta;
  client.publish("/drone/status", mensagemStatus.c_str());
  client.publish("/alerta/confirmado", tipoAlerta.c_str());

  Serial.print("ALERTA CONFIRMADO: ");
  Serial.println(tipoAlerta);
}

void desativarDrone(bool normalizacao) {
  emMissao = false;
  emergenciaAtiva = false;
  digitalWrite(LED_DRONE, LOW);
  noTone(BUZZER);

  client.publish("/drone/status", "base");
  if (normalizacao) {
    client.publish("/alerta/cancelado", tipoEmergencia.c_str());
    Serial.print("EMERGÊNCIA ENCERRADA: ");
    Serial.println(tipoEmergencia);
  } else {
    Serial.println("Drone retornou à base (tempo limite)");
  }

  tipoEmergencia = "";
}

// ================= HANDLER MQTT =================
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("]: ");

  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  if (String(topic) == "/drone/controle") {
    if (messageTemp == "ativar") ativarDrone("manual");
    else if (messageTemp == "desativar") desativarDrone();
  }
}
