/*
 * TC4 - Control de 4 LEDs por WebSocket y JSON
 * Diseño de Interfaces (I7262) - Dr. Rubén Estrada Marmolejo
 * Alumna: Iridia Guadalupe Salazar Esmerio
 *
 * Arquitectura: ESP32-S3 (Servidor WS: 81) -> 4 LEDs
 * WiFi: Conexión local a la red.
 */
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

const char* nombreRed = "IRIDIA'S PHONE";
const char* claveRed  = "tumamaxd";

// Pines de los 4 LEDs indicados en el reporte (Canal 1 a 4)
const int pinesLeds[4] = {10, 11, 12, 13};
bool estadoLeds[4] = {false, false, false, false};

const int pinLedRgb = 48; // Neopixel del ESP32-S3

WebSocketsServer socketServidor(81);
Adafruit_NeoPixel ledPlaca(1, pinLedRgb, NEO_GRB + NEO_KHZ800);

// ---- Helpers JSON ----
void enviarJsonACliente(uint8_t idCliente, const JsonDocument& doc) {
  String salida; 
  serializeJson(doc, salida);
  socketServidor.sendTXT(idCliente, salida);
}

void difundirJson(const JsonDocument& doc) {
  String salida; 
  serializeJson(doc, salida);
  socketServidor.broadcastTXT(salida);
}

void aplicarLeds() {
  for(int i=0; i<4; i++) {
    digitalWrite(pinesLeds[i], estadoLeds[i] ? HIGH : LOW);
  }
  // Feedback visual en la placa: verde si hay algún LED prendido, si no, azul oscuro
  bool algunEncendido = estadoLeds[0] || estadoLeds[1] || estadoLeds[2] || estadoLeds[3];
  ledPlaca.setPixelColor(0, algunEncendido ? ledPlaca.Color(0, 80, 0) : ledPlaca.Color(0, 0, 20));
  ledPlaca.show();
}

void difundirEstado() {
  JsonDocument doc;
  doc["tipo"] = "estado";
  JsonArray arr = doc["leds"].to<JsonArray>();
  for(int i=0; i<4; i++) {
    arr.add(estadoLeds[i]);
  }
  difundirJson(doc);
}

void onEventoWS(uint8_t idCliente, WStype_t tipo, uint8_t *payload, size_t longitud) {
  if (tipo == WStype_CONNECTED) {
    Serial.printf("[%u] Cliente WebSocket conectado.\n", idCliente);
    
    // 1. Enviar saludo inicial
    JsonDocument saludo;
    saludo["tipo"]    = "saludo";
    saludo["mensaje"] = "Servidor TC4 listo";
    saludo["puerto"]  = 81;
    enviarJsonACliente(idCliente, saludo);

    // 2. Enviar el estado actual de los LEDs
    JsonDocument est;
    est["tipo"] = "estado";
    JsonArray arr = est["leds"].to<JsonArray>();
    for(int i=0; i<4; i++) arr.add(estadoLeds[i]);
    enviarJsonACliente(idCliente, est);
    
  } 
  else if (tipo == WStype_TEXT) {
    // Procesar mensajes JSON desde Qt usando 'longitud' para mayor seguridad
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, longitud);

    if (error) {
      Serial.print("Error al parsear JSON: ");
      Serial.println(error.c_str());
      return; 
    }

    if (!doc.containsKey("tipo")) return; // Ignorar mensajes mal formados

    String tipoMsg = doc["tipo"].as<String>();

    if (tipoMsg == "salida") {
      int canal = doc["canal"].as<int>();
      bool encendido = doc["encendido"].as<bool>();
      
      if (canal >= 1 && canal <= 4) {
        estadoLeds[canal - 1] = encendido;
        aplicarLeds();
        difundirEstado(); // Confirmar el cambio a la PC
        Serial.printf("Canal %d -> %s\n", canal, encendido ? "ON" : "OFF");
      } else {
        Serial.println("Error: Canal fuera de rango (1-4)");
      }
    } 
    else if (tipoMsg == "leds") {
      JsonArray arr = doc["estado"].as<JsonArray>();
      for(int i=0; i<4; i++) {
        estadoLeds[i] = arr[i].as<bool>();
      }
      aplicarLeds();
      difundirEstado();
      Serial.println("Actualización de los 4 LEDs simultáneos recibida.");
    }
    else if (tipoMsg == "consulta") {
      difundirEstado();
    }
  }
}

void conectarRed() {
  Serial.print("Conectando a "); Serial.print(nombreRed);
  WiFi.mode(WIFI_STA);
  WiFi.begin(nombreRed, claveRed);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300); Serial.print(".");
    ledPlaca.setPixelColor(0, ledPlaca.Color(60, 40, 0)); ledPlaca.show(); delay(120);
    ledPlaca.setPixelColor(0, 0);                         ledPlaca.show();
  }
  Serial.println();
  Serial.print("Conectado. IP: "); Serial.println(WiFi.localIP());
  Serial.println("WebSocket en ws://<IP>:81");
}

void setup() {
  Serial.begin(115200);
  
  // Configurar los 4 pines de salida
  for(int i=0; i<4; i++) {
    pinMode(pinesLeds[i], OUTPUT);
  }
  
  ledPlaca.begin(); ledPlaca.setBrightness(60); ledPlaca.show();
  aplicarLeds();
  conectarRed();
  
  socketServidor.begin();
  socketServidor.onEvent(onEventoWS);
  Serial.println("Servidor TC4 iniciado. Esperando conexión desde Qt...");
}

void loop() {
  socketServidor.loop();
  // El código no necesita polling en el loop porque los eventos vienen directo por onEventoWS
}
