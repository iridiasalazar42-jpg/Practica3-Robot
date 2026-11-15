#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char* nombreRed  = "IRIDIA'S PHONE";
const char* claveRed   = "tumamaxd";

WebSocketsServer servidorSocket = WebSocketsServer(81);

const int pinServo = 14;
const int pulsoMinimo = 500;  
const int pulsoMaximo = 2400; 

const int frecuenciaPWM = 50; 
const int resolucionPWM = 14; 

const int dutyMinimo = (pulsoMinimo * 16383) / 20000; 
const int dutyMaximo = (pulsoMaximo * 16383) / 20000; 

const int pinPot = 8;
const unsigned long periodoPot = 100;
unsigned long marcaTiempoPot = 0;

const int anguloMinimo = 0;
const int anguloMaximo = 180;
int anguloActual = 90;

bool barridoActivo = false;
int  direccionBarrido = +1;
const int incrementoBarrido = 5;
const unsigned long pasoBarrido = 40;
unsigned long marcaTiempoBarrido = 0;

int recortarAngulo(int angulo) {
  if (angulo < anguloMinimo) return anguloMinimo;
  if (angulo > anguloMaximo) return anguloMaximo;
  return angulo;
}

void aplicarAngulo(int angulo) {
  anguloActual = recortarAngulo(angulo);
  int duty = map(anguloActual, 0, 180, dutyMinimo, dutyMaximo);
  ledcWrite(pinServo, duty);
}

void avisarAngulo() {
  JsonDocument documento;
  documento["tipo"]   = "servo_ack";
  documento["angulo"] = anguloActual;
  String texto;
  serializeJson(documento, texto);
  servidorSocket.broadcastTXT(texto);
}

void enviarError(uint8_t numCliente, const char* mensaje) {
  JsonDocument documento;
  documento["tipo"]    = "error";
  documento["mensaje"] = mensaje;
  String texto;
  serializeJson(documento, texto);
  servidorSocket.sendTXT(numCliente, texto);
  Serial.println(mensaje);
}

void procesarComando(uint8_t numCliente, uint8_t* carga, size_t longitud) {
  JsonDocument documento;
  DeserializationError error = deserializeJson(documento, carga, longitud);
  if (error) { enviarError(numCliente, "JSON invalido"); return; }

  const char* tipoMsg = documento["tipo"];
  if (tipoMsg == nullptr) { enviarError(numCliente, "Falta el campo \"tipo\""); return; }

  if (strcmp(tipoMsg, "servo") == 0) {
    JsonVariant campoAngulo = documento["angulo"];
    if (!campoAngulo.is<int>() && !campoAngulo.is<float>()) {
      enviarError(numCliente, "Falta el campo numerico \"angulo\""); return;
    }
    int pedido = (int)lround(campoAngulo.as<float>());
    barridoActivo = false;
    aplicarAngulo(pedido);
    avisarAngulo();
  }
  else if (strcmp(tipoMsg, "barrido") == 0) {
    barridoActivo = true;
    direccionBarrido = +1;
    aplicarAngulo(anguloMinimo);
    marcaTiempoBarrido = millis();
    avisarAngulo();
  }
  else if (strcmp(tipoMsg, "consulta") == 0) {
    avisarAngulo();
  }
  else {
    enviarError(numCliente, "Tipo desconocido");
  }
}

void alEventoWebSocket(uint8_t numCliente, WStype_t tipo, uint8_t* carga, size_t longitud) {
  switch (tipo) {
    case WStype_CONNECTED: {
      Serial.println("Cliente conectado");
      JsonDocument saludo;
      saludo["tipo"]    = "saludo";
      saludo["mensaje"] = "Servidor TC05 listo";
      saludo["puerto"]  = 81;
      saludo["angulo"]  = anguloActual;
      String textoSaludo;
      serializeJson(saludo, textoSaludo);
      servidorSocket.sendTXT(numCliente, textoSaludo);
      break;
    }
    case WStype_TEXT:
      procesarComando(numCliente, carga, longitud);
      break;
    default: break;
  }
}

void actualizarBarrido() {
  if (!barridoActivo) return;
  if (millis() - marcaTiempoBarrido < pasoBarrido) return;
  marcaTiempoBarrido = millis();

  int siguiente = anguloActual + direccionBarrido * incrementoBarrido;
  if (siguiente >= anguloMaximo) { siguiente = anguloMaximo; direccionBarrido = -1; }
  if (siguiente <= anguloMinimo && direccionBarrido < 0) { siguiente = anguloMinimo; barridoActivo = false; }
  aplicarAngulo(siguiente);
  avisarAngulo();
}

void enviarPotenciometro() {
  if (millis() - marcaTiempoPot < periodoPot) return;
  marcaTiempoPot = millis();
  if (servidorSocket.connectedClients() == 0) return;

  int lectura = analogRead(pinPot);
  float voltaje = lectura * 3.3 / 4095.0;
  JsonDocument documento;
  documento["tipo"]    = "pot";
  documento["valor"]   = lectura;
  documento["voltaje"] = round(voltaje * 100) / 100.0;
  String texto;
  serializeJson(documento, texto);
  servidorSocket.broadcastTXT(texto);
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(pinPot, ADC_11db);

  ledcAttach(pinServo, frecuenciaPWM, resolucionPWM);
  aplicarAngulo(anguloActual);

  WiFi.begin(nombreRed, claveRed);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println(WiFi.localIP());

  servidorSocket.begin();
  servidorSocket.onEvent(alEventoWebSocket);
}

void loop() {
  servidorSocket.loop();
  actualizarBarrido();
  enviarPotenciometro();
}
