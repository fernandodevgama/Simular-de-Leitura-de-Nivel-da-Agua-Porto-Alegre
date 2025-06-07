#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- CONFIGURAÇÕES DO SENSOR ---
#define trigPin 23
#define echoPin 22

// --- CONFIGURAÇÕES DE REDE E API ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "https://api.inovagama.com.br/webhook/88227668-393b-4129-a8dd-df55ad130538";

// --- CONFIGURAÇÕES DE TEMPO PARA O LOTE ---
const int INTERVALO_LEITURA_MS = 2000;   // Ler o sensor a cada 2 segundos
const int INTERVALO_ENVIO_MS = 30000;   // Enviar o lote a cada 30 segundos
unsigned long ultimaLeitura = 0;
unsigned long ultimoEnvio = 0;

// --- ESTRUTURA GLOBAL PARA ARMAZENAR O LOTE ---
JsonDocument doc;
JsonArray leiturasArray;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Prepara o documento para ser um array JSON
  leiturasArray = doc.to<JsonArray>();

  // Conexão Wi-Fi
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");
  Serial.println("Iniciando coleta de dados em lote...");
}

void loop() {
  unsigned long agora = millis();

  // --- Bloco 1: Coleta de dados a cada 2 segundos ---
  if (agora - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora; // Reseta o timer da leitura

    // Faz a leitura do sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    float nivelEmMetros = (500.0 - (pulseIn(echoPin, HIGH) * 0.034 / 2)) / 100.0;
    nivelEmMetros = round(nivelEmMetros * 100.0) / 100.0;

    // Adiciona a leitura como um novo objeto dentro do nosso array
    JsonObject leituraObj = leiturasArray.add<JsonObject>();
    leituraObj["altura"] = nivelEmMetros;

    Serial.print("Coletado (item ");
    Serial.print(leiturasArray.size());
    Serial.print("): altura = ");
    Serial.println(nivelEmMetros);
  }

  // --- Bloco 2: Envio do lote a cada 30 segundos ---
  if (agora - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = agora; // Reseta o timer do envio

    // Só envia se o array não estiver vazio e o WiFi estiver conectado
    if (!leiturasArray.isNull() && leiturasArray.size() > 0 && WiFi.status() == WL_CONNECTED) {
      String jsonPayload;
      serializeJson(doc, jsonPayload);

      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      Serial.println("\n--- ENVIANDO LOTE PARA API ---");
      Serial.println(jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        Serial.print("Resposta da API: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Erro no envio do lote! Código: ");
        Serial.println(httpResponseCode);
      }
      http.end();

      // **MUITO IMPORTANTE: Limpa o array para começar a próxima coleta**
      leiturasArray.clear();
      Serial.println("Lote enviado. Iniciando nova coleta.");
      Serial.println("--------------------------------\n");
    }
  }
}
