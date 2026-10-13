/*
 * TA3 - DataCenter Andino S.A.C.
 * ESP32 + DHT22 + LDR -> MQTT (Mosquitto)
 *
 * Publica cada 5 s un JSON con temperatura, humedad y luz.
 * Usa Last Will (LWT) para que el broker publique "offline" si el ESP32 se cae.
 *
 * Librerias (libraries.txt): PubSubClient, DHT sensor library, Adafruit Unified Sensor
 */
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>
#include <sys/time.h>

// ---------- CONFIGURACION ----------
const char* WIFI_SSID = "Wokwi-GUEST";   // red WiFi del simulador Wokwi
const char* WIFI_PASS = "";

// Host/puerto donde Wokwi (nube) pueda alcanzar tu Mosquitto.
// Ver README, seccion "Conectar Wokwi con Mosquitto".
const char* MQTT_HOST = "CAMBIAR_POR_HOST_DEL_BROKER";
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "";              // vacio si allow_anonymous true
const char* MQTT_PASS = "";

const char* DEVICE_ID    = "esp32-sala01";
const char* TOPIC_TEL    = "dcandino/sala01/telemetria";
const char* TOPIC_ESTADO = "dcandino/sala01/estado";

const unsigned long INTERVALO_MS = 5000;

#define DHT_PIN  15
#define DHT_TIPO DHT22
#define LDR_PIN  34

// Calibracion del LDR (ADC 12 bits: 0..4095).
// En el modulo LDR de Wokwi, un valor ADC ALTO = MAS OSCURO, por eso se invierte.
// Ajusta mirando el monitor serie mientras mueves el control de luz.
const bool LDR_INVERTIDO = true;
// -----------------------------------

DHT dht(DHT_PIN, DHT_TIPO);
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long ultimoEnvio = 0;
unsigned long ultimoIntentoMqtt = 0;
uint32_t seq = 0;

void conectarWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(" OK");
}

bool conectarMqtt() {
  Serial.print("Conectando a MQTT... ");
  const char* user = strlen(MQTT_USER) ? MQTT_USER : NULL;
  const char* pass = strlen(MQTT_USER) ? MQTT_PASS : NULL;
  // clientId, user, pass, willTopic, willQoS, willRetain, willMessage
  bool ok = mqtt.connect(DEVICE_ID, user, pass, TOPIC_ESTADO, 1, true, "offline");
  if (ok) {
    Serial.println("OK");
    mqtt.publish(TOPIC_ESTADO, "online", true);   // retained
  } else {
    Serial.printf("fallo, rc=%d\n", mqtt.state());
  }
  return ok;
}

long long ahoraMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (tv.tv_sec < 1700000000) return 0;           // NTP aun no sincronizo
  return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

int luzPorcentaje(int raw) {
  int pct = map(raw, 0, 4095, 0, 100);
  if (LDR_INVERTIDO) pct = 100 - pct;
  return constrain(pct, 0, 100);
}

void publicarLectura() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int raw = analogRead(LDR_PIN);
  int pct = luzPorcentaje(raw);

  // Si el DHT22 falla (NaN) se publica null: Node-RED lo detecta como lectura invalida.
  char tStr[12], hStr[12];
  if (isnan(t)) strcpy(tStr, "null"); else snprintf(tStr, sizeof(tStr), "%.1f", t);
  if (isnan(h)) strcpy(hStr, "null"); else snprintf(hStr, sizeof(hStr), "%.1f", h);

  char payload[256];
  snprintf(payload, sizeof(payload),
           "{\"device_id\":\"%s\",\"seq\":%lu,\"ts\":%lld,\"temp\":%s,\"hum\":%s,\"luz\":%d,\"luz_pct\":%d}",
           DEVICE_ID, (unsigned long)seq, ahoraMs(), tStr, hStr, raw, pct);

  if (mqtt.publish(TOPIC_TEL, payload, false)) {
    Serial.printf("PUB %s\n", payload);
    seq++;
  } else {
    Serial.println("Error al publicar");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LDR_PIN, INPUT);
  conectarWiFi();
  configTime(0, 0, "pool.ntp.org");               // hora UTC para medir latencia
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);
  conectarMqtt();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();

  if (!mqtt.connected()) {
    if (millis() - ultimoIntentoMqtt > 3000) {     // reintento no bloqueante
      ultimoIntentoMqtt = millis();
      conectarMqtt();
    }
  } else {
    mqtt.loop();
    if (millis() - ultimoEnvio >= INTERVALO_MS) {
      ultimoEnvio = millis();
      publicarLectura();
    }
  }
}
