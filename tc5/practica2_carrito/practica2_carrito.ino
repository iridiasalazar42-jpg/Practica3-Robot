#include <WiFi.h>
#include <WebSocketsServer.h>   // arduinoWebSockets (Links2004)
#include <ArduinoJson.h>        // ArduinoJson 7.x (Benoit Blanchon)
#include <ESPmDNS.h>            // mDNS / DNS-SD
#include <Adafruit_NeoPixel.h>
#include <DHT.h>

// ==== FreeRTOS (tareas/colas) ====
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ----------------------------------------------------------------------
//                 PROTOTIPOS
// ----------------------------------------------------------------------
void enviarJsonACliente(uint8_t idCliente, const JsonDocument& doc);
void difundirJson(const JsonDocument& doc);

// ====== CONFIGURACIÓN GENERAL ======
#define SIMULAR_DHT true   // true = datos simulados / false = leer DHT22 real

#ifndef PIN_RGB
#define PIN_RGB 48         // Pin para LED RGB (NeoPixel)
#endif

Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

// ====== CONFIG WiFi ======
const char* ssid     = "IRIDIA'S PHONE";
const char* password = "tumamaxd";

// ====== Servidor WebSocket ======
WebSocketsServer servidorWS(81);
const int PIN_LED = 44;

// ====== Temporizadores y variables ======
volatile bool latidoRecibido = false;
unsigned long ultimoEnvioMs  = 0;
unsigned long CADA_MS        = 2000;

// ====== DHT22 ======
// CORRECCIÓN: Se cambió al pin 9 para evitar choque con AIN2 (pin 6)
#define PIN_DHT   9
#define DHTTYPE   DHT22
DHT dht(PIN_DHT, DHTTYPE);

const unsigned long DHT_MIN_MS_REAL = 2000;
const unsigned long DHT_MIN_MS_SIM  = 500;
float tempC_cache = NAN;
float hum_cache   = NAN;
bool  dht_ok      = false;
unsigned long ultimoDhtMs = 0;

// ====== ADC opcional ======
// CORRECCIÓN: Se cambió al pin 8 para evitar choque con PWMA (pin 4)
const int PIN_ADC = 8;
String nombreHost;

// ====== HC-SR04 (PRÁCTICA 2) ======
#define PIN_TRIG 12
#define PIN_ECHO 13
unsigned long ultimoMsUS = 0;
const unsigned long CADA_MS_US = 100; // Muestreo rápido para la gráfica

// ======================================================================
//              MAPEO DE PINES – DRIVER TB6612FNG (Módulo Rojo)
// ======================================================================
#define STBY 10 // Pin para encender el puente H

// Motor A (Lado Izquierdo)
#define PWMA 4  // Velocidad Motor Izquierdo
#define AIN1 5  // Dirección 1 Motor Izquierdo
#define AIN2 6  // Dirección 2 Motor Izquierdo

// Motor B (Lado Derecho)
#define PWMB 7  // Velocidad Motor Derecho
#define BIN1 15 // Dirección 1 Motor Derecho
#define BIN2 16 // Dirección 2 Motor Derecho

// ======================================================================
//                     INICIALIZACIÓN DE PINES
// ======================================================================
static inline void motores_init_pines() {
  pinMode(STBY, OUTPUT);
  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  digitalWrite(STBY, HIGH); // Encender el driver

  digitalWrite(PWMA, LOW); digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(PWMB, LOW); digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

// ======================================================================
//                 HELPERS DE MOVIMIENTO 
// ======================================================================
static inline void motorA_stop()     { digitalWrite(PWMA, LOW);  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, LOW); }
static inline void motorA_adelante() { digitalWrite(PWMA, HIGH); digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); }
static inline void motorA_atras()    { digitalWrite(PWMA, HIGH); digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH); }

static inline void motorB_stop()     { digitalWrite(PWMB, LOW);  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, LOW); }

// SI LAS RUEDAS DERECHAS GIRAN AL REVÉS, INVIERTE HIGH/LOW EN ESTAS DOS LÍNEAS:
static inline void motorB_adelante() { digitalWrite(PWMB, HIGH); digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); }
static inline void motorB_atras()    { digitalWrite(PWMB, HIGH); digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH); }

static inline void motores_parar_total() { motorA_stop(); motorB_stop(); }

static inline void mover_avanzar()    { motorA_adelante(); motorB_adelante(); }
static inline void mover_retroceder() { motorA_atras();    motorB_atras();    }
static inline void mover_izquierda()  { motorA_atras();    motorB_adelante(); } 
static inline void mover_derecha()    { motorA_adelante(); motorB_atras();    } 

// ======================================================================
//         COLA / TASK DE MOTORES (CORE 1)
// ======================================================================
enum class DirMov : uint8_t { Parado=0, Avanzar, Retroceder, Izquierda, Derecha, Parar };

struct CmdMotor {
  DirMov accion;
  uint32_t dur_ms;
  uint32_t t0_ms;
};

static QueueHandle_t motorCmdQ = nullptr;
static TaskHandle_t motorTaskHandle = nullptr;

void MotorTask(void* arg) {
  (void)arg;
  CmdMotor cmd{DirMov::Parado, 0, 0};
  bool enMovimiento = false;
  uint32_t t_fin = 0;

  for (;;) {
    CmdMotor nuevo;
    if (xQueueReceive(motorCmdQ, &nuevo, (TickType_t)0) == pdTRUE) {
      enMovimiento = false;
      motores_parar_total();

      if (nuevo.accion == DirMov::Parar || nuevo.accion == DirMov::Parado || nuevo.dur_ms == 0) {
        enMovimiento = false;
      } else {
        switch (nuevo.accion) {
          case DirMov::Avanzar:     mover_avanzar();    break;
          case DirMov::Retroceder:  mover_retroceder(); break;
          case DirMov::Izquierda:   mover_izquierda();  break;
          case DirMov::Derecha:     mover_derecha();    break;
          default:                  motores_parar_total(); break;
        }
        nuevo.t0_ms = millis();
        t_fin = nuevo.t0_ms + nuevo.dur_ms;
        enMovimiento = true;

        JsonDocument ack;
        ack["tipo"] = "mover_inicio";
        difundirJson(ack);
      }
      cmd = nuevo;
    }

    if (enMovimiento) {
      uint32_t ahora = millis();
      if ((int32_t)(ahora - t_fin) >= 0) {
        enMovimiento = false;
        motores_parar_total();
        JsonDocument fin;
        fin["tipo"] = "mover_fin";
        difundirJson(fin);
      }
    }
    vTaskDelay(1); 
  }
}

// ======================================================================
//                         AUXILIARES
// ======================================================================
void setColor(uint8_t r, uint8_t g, uint8_t b) {
  rgb.setPixelColor(0, rgb.Color(r, g, b));
  rgb.show();
}

String macSinDosPuntos() {
  String mac = WiFi.macAddress();
  mac.toLowerCase();
  String out;
  for (char c : mac) if (c != ':') out += c;
  return out;
}
String sufijoMAC6() {
  String m = macSinDosPuntos();
  return (m.length() >= 6) ? m.substring(m.length() - 6) : m;
}

void enviarJsonACliente(uint8_t idCliente, const JsonDocument& doc) {
  String out; serializeJson(doc, out);
  servidorWS.sendTXT(idCliente, out);
}
void difundirJson(const JsonDocument& doc) {
  String out; serializeJson(doc, out);
  servidorWS.broadcastTXT(out);
}

// ======================================================================
//                         LECTURA DHT
// ======================================================================
void actualizarLecturaDHT(bool forzar = false) {
  unsigned long ahora = millis();
  unsigned long intervalo = SIMULAR_DHT ? DHT_MIN_MS_SIM : DHT_MIN_MS_REAL;
  if (!forzar && (ahora - ultimoDhtMs < intervalo)) return;

  if (SIMULAR_DHT) {
    tempC_cache = 25.0 + (random(-300, 300) / 100.0);
    hum_cache   = 45.0 + (random(-1000, 1000) / 100.0);
    dht_ok = true;
    ultimoDhtMs = ahora;
  } else {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      dht_ok = false;
    } else {
      hum_cache = h;
      tempC_cache = t;
      dht_ok = true;
      ultimoDhtMs = ahora;
    }
  }
}

// ======================================================================
//                 LECTURA HC-SR04 (PRÁCTICA 2)
// ======================================================================
float leerUltrasonico(bool &ok) {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 30000); // timeout 30ms

  if (duracion == 0) {
    ok = false;
    return 0.0;
  }

  // d[cm] = (t_echo[us] * 0.034) / 2
  float distancia = (duracion * 0.034) / 2.0;

  // Rango útil del sensor: 2 a 400 cm
  if (distancia >= 2.0 && distancia <= 400.0) {
    ok = true;
    return distancia;
  } else {
    ok = false;
    return 0.0;
  }
}

// ======================================================================
//                     EVENTOS WEBSOCKET
// ======================================================================
void onEventoWS(uint8_t idCliente, WStype_t tipo, uint8_t * payload, size_t longitud) {
  switch (tipo) {
    case WStype_CONNECTED: {
      IPAddress ip = servidorWS.remoteIP(idCliente);
      Serial.printf("[WS] Cliente %u conectado desde %s\n", idCliente, ip.toString().c_str());

      JsonDocument msg;
      msg["tipo"]        = "saludo";
      msg["mensaje"]     = "Servidor ESP32-S3 listo";
      msg["puerto"]      = 81;
      msg["nombre_host"] = nombreHost;
      msg["mac"]         = WiFi.macAddress();
      enviarJsonACliente(idCliente, msg);
    } break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Cliente %u desconectado\n", idCliente);
      break;

    case WStype_TEXT: {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, payload, longitud);
      if (err) return;

      const char* tipoMsg = doc["tipo"] | "";

      if (strcmp(tipoMsg, "latido") == 0) {
        latidoRecibido = true;
        JsonDocument ack;
        ack["tipo"]       = "latido_ok";
        ack["tiempo_ms"]  = (uint32_t)millis();
        enviarJsonACliente(idCliente, ack);
      }
      else if (strcmp(tipoMsg, "leer_dht") == 0) {
        actualizarLecturaDHT(true);
        JsonDocument resp;
        resp["tipo"]        = "dht_lectura";
        resp["dht_ok"]      = dht_ok;
        resp["temperatura"] = tempC_cache;
        resp["humedad"]     = hum_cache;
        enviarJsonACliente(idCliente, resp);
      }
      else if (strcmp(tipoMsg, "led") == 0) {
        int estado = doc["estado"] | 0;
        digitalWrite(PIN_LED, estado ? HIGH : LOW);
        JsonDocument resp;
        resp["tipo"] = "led_ack";
        resp["estado"] = estado;
        enviarJsonACliente(idCliente, resp);
      }
      else if (strcmp(tipoMsg, "rgb") == 0) {
        uint8_t r = doc["r"] | 0;
        uint8_t g = doc["g"] | 0;
        uint8_t b = doc["b"] | 0;
        setColor(r, g, b);
        JsonDocument ack;
        ack["tipo"] = "rgb_ack";
        ack["r"] = r; ack["g"] = g; ack["b"] = b;
        enviarJsonACliente(idCliente, ack);
      }
      else if (strcmp(tipoMsg, "mover") == 0) {
        const char* accionStr = doc["accion"] | "avanzar";
        uint32_t ms = doc["ms"] | 600;
        if (ms > 8000) ms = 8000; 

        DirMov a = DirMov::Avanzar;
        if      (strcmp(accionStr, "retroceder") == 0) a = DirMov::Retroceder;
        else if (strcmp(accionStr, "izquierda")  == 0) a = DirMov::Izquierda;
        else if (strcmp(accionStr, "derecha")    == 0) a = DirMov::Derecha;
        else if (strcmp(accionStr, "parar")      == 0) a = DirMov::Parar;

        CmdMotor cmd{ a, ms, 0 };
        bool ok = (motorCmdQ && (xQueueSend(motorCmdQ, &cmd, 0) == pdTRUE));

        JsonDocument ack2;
        ack2["tipo"]   = ok ? "mover_ack" : "mover_err";
        ack2["accion"] = accionStr;
        ack2["ms"]     = ms;
        enviarJsonACliente(idCliente, ack2);
      }
    } break;
    default: break;
  }
}

// ======================================================================
//                               mDNS
// ======================================================================
void iniciarMDNSyDNSSD() {
  nombreHost = "esp32-" + sufijoMAC6();
  nombreHost.toLowerCase();

  if (MDNS.begin(nombreHost.c_str())) {
    MDNS.addService("ws", "tcp", 81);
    MDNS.addServiceTxt("ws", "tcp", "proto", "json");
    Serial.println("[mDNS] Iniciado: " + nombreHost + ".local");
  } else {
    Serial.println("[mDNS] Error iniciando mDNS");
  }
}

// ======================================================================
//                               SETUP
// ======================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  // HC-SR04 Pines
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_ADC, INPUT);
  analogReadResolution(12);
  randomSeed(analogRead(PIN_ADC));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  rgb.begin();
  rgb.clear();
  rgb.show();

  if (!SIMULAR_DHT) dht.begin();

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  iniciarMDNSyDNSSD();

  motores_init_pines();
  motores_parar_total();

  motorCmdQ = xQueueCreate(30, sizeof(CmdMotor));
  if (!motorCmdQ) {
    Serial.println("[MOTOR] ERROR: no se pudo crear la cola de comandos");
  } else {
    BaseType_t ok = xTaskCreatePinnedToCore(
      MotorTask,
      "MotorTask",
      4096,        
      nullptr,
      2,          
      &motorTaskHandle,
      1            
    );
    if (ok != pdPASS) {
      Serial.println("[MOTOR] ERROR: no se pudo crear el task de motores");
    } else {
      Serial.println("[MOTOR] Task de motores iniciado en core 1");
    }
  }

  servidorWS.begin();
  servidorWS.onEvent(onEventoWS);
  Serial.println("Servidor WebSocket en ws://" + nombreHost + ".local:81");
}

// ======================================================================
//                               LOOP
// ======================================================================
void loop() {
  servidorWS.loop();

  unsigned long ahora = millis();
  actualizarLecturaDHT(false);

  // 1) Lógica de la Práctica 2: Ultrasonido rápido sin bloquear
  if (ahora - ultimoMsUS >= CADA_MS_US) {
    ultimoMsUS = ahora;
    bool ok_us = false;
    float dist = leerUltrasonico(ok_us);

    JsonDocument docUS;
    docUS["ok"]   = ok_us;
    docUS["dist"] = (round(dist * 100) / 100.0); // Limitado a 2 decimales

    // Enviar JSON puro por Serial (Requisito Práctica 2)
    String jsonStr;
    serializeJson(docUS, jsonStr);
    Serial.println(jsonStr);

    // Adjuntar a red: Agregar "tipo" y transmitir por WebSocket a todos los clientes (Práctica 1)
    docUS["tipo"] = "ultrasonico";
    difundirJson(docUS);
  }

  // 2) Lógica de la Práctica 1: Telemetría de DHT/ADC y Latido
  if (latidoRecibido && (ahora - ultimoEnvioMs >= CADA_MS)) {
    ultimoEnvioMs = ahora;

    int lecturaADC = analogRead(PIN_ADC);

    JsonDocument msg;
    msg["tipo"]         = "telemetria";
    msg["adc"]          = lecturaADC;
    msg["temperatura"]  = tempC_cache;
    msg["humedad"]      = hum_cache;
    msg["dht_ok"]       = dht_ok;
    msg["tiempo_ms"]    = (uint32_t)ahora;

    difundirJson(msg);
  }
}
