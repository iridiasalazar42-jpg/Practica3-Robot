/*
 * ============================================================================
 *  TC05 - Diseño de Interfaces (I7262) - Firmware para ESP32-S3-DevKitC-1
 *  Control de un servomotor SG90 desde Qt por WebSocket (JSON)
 * ----------------------------------------------------------------------------
 *  Igual que en el TC4, la ESP32-S3 funciona como SERVIDOR WebSocket (puerto
 *  81) y la interfaz de Qt se conecta como CLIENTE. Ahora, en lugar de
 *  encender LEDs, la interfaz manda un ANGULO (0 a 180 grados) y la ESP32 mueve
 *  un servomotor SG90 a esa posicion. Despues CONFIRMA el angulo que aplico
 *  ("servo_ack"); la interfaz muestra ese valor confirmado, que puede ser
 *  distinto del que se pidio (por ejemplo, si se pidio 250 se aplica 180).
 *
 *  Librerias:
 *   - WebSockets  (Markus Sattler / Links2004)  -> servidor WebSocket
 *   - ArduinoJson (Benoit Blanchon, v7)          -> armar/leer JSON
 *   - ESP32Servo  (Kevin Harrington / madhephaestus) -> señal PWM del servo
 *
 *  Placa (Arduino IDE): "ESP32S3 Dev Module".
 *
 *  ¿COMO SE CONTROLA UN SERVO?
 *  ---------------------------
 *  El SG90 recibe un pulso cada 20 ms (50 Hz). El ANCHO del pulso indica la
 *  posicion:  ~500 us -> 0 grados,  ~1450 us -> 90 grados,  ~2400 us -> 180.
 *  La libreria ESP32Servo genera esa señal con el periferico PWM (LEDC) de la
 *  ESP32, asi que el loop() no se bloquea: basta con llamar servo.write(angulo).
 *
 *  PROTOCOLO JSON  (mismo esquema del TC4: campo "tipo")
 *  --------------
 *  Computadora  ->  ESP32 (comandos):
 *     {"tipo":"servo", "angulo":90}     // mover el servo (0..180, se recorta)
 *     {"tipo":"barrido"}                // demo: 0 -> 180 -> 0 sin bloquear
 *     {"tipo":"consulta"}               // pedir el angulo actual
 *
 *  ESP32  ->  Computadora (saludo, confirmaciones y errores):
 *     {"tipo":"saludo",    "mensaje":"Servidor TC05 listo", "puerto":81, "angulo":90}
 *     {"tipo":"servo_ack", "angulo":90}             // a TODOS los clientes
 *     {"tipo":"error",     "mensaje":"..."}         // solo al que se equivoco
 *     {"tipo":"pot", "valor":2048, "voltaje":1.65}  // cada 100 ms, a TODOS
 *
 *  POTENCIOMETRO (para la grafica de Qt Charts): extremos a 3V3 y GND, cursor
 *  (pin central) al GPIO 8, que es una entrada analogica del ADC1.
 * ============================================================================
 */

#include <WiFi.h>
#include <WebSocketsServer.h>   // WebSockets (Links2004)
#include <ArduinoJson.h>        // ArduinoJson v7 (Benoit Blanchon)
#include <ESP32Servo.h>         // ESP32Servo (señal de 50 Hz para servos)

// ---------------------------------------------------------------------------
// 1) Credenciales del punto de acceso (Access Point del salon)
// ---------------------------------------------------------------------------
const char* nombreRed  = "IRIDIA'S PHONE";      // SSID del Access Point
const char* claveRed    = "tumamaxd";   // Contraseña del Access Point

// ---------------------------------------------------------------------------
// 2) Servidor WebSocket en el puerto 81
// ---------------------------------------------------------------------------
WebSocketsServer servidorSocket = WebSocketsServer(81);

// ---------------------------------------------------------------------------
// 3) Pin del servo: GPIO 14
//    ¿Por que el 14?
//     - En la ESP32-S3-DevKitC-1 los ultimos pines del conector J1 son
//       GPIO14, 5V y GND, en ese orden: el conector del SG90 (naranja, rojo,
//       cafe) entra DIRECTO, sin protoboard ni cables extra.
//     - NO es pin de arranque (strapping): 0, 3, 45 y 46 cambian el modo de
//       arranque si tienen algo conectado al encender.
//     - NO lo usa la PSRAM octal: en los modulos N8R8 / N16R8 los GPIO 35, 36
//       y 37 estan ocupados por la memoria.
//     - NO es el USB nativo (GPIO 19 y 20).
//     - NO choca con el TC4 (salidas 4..7, botones 0, 15 y 16, LED RGB 48),
//       asi que el mismo protoboard del TC4 puede quedarse armado.
// ---------------------------------------------------------------------------
const int pinServo = 14;

// Ancho de pulso del SG90 (microsegundos) para 0 y 180 grados.
// Si tu servo no llega a los extremos o "zumba" en ellos, ajusta estos valores.
const int pulsoMinimo = 500;    // 0 grados
const int pulsoMaximo = 2400;   // 180 grados

// ---------------------------------------------------------------------------
// Potenciometro en el GPIO 8 (ADC1, canal 7). 12 bits: 0..4095 -> 0..3.3 V
// ---------------------------------------------------------------------------
const int pinPot = 8;
const unsigned long periodoPot = 100;     // milisegundos entre lecturas (10 por segundo)
unsigned long marcaTiempoPot = 0;

const int anguloMinimo = 0;
const int anguloMaximo = 180;

Servo servo;                    // objeto que controla el servomotor
int anguloActual = 90;          // posicion aplicada (arranca al centro)

// ---------------------------------------------------------------------------
// 4) Barrido de demostracion SIN delay(): una "maquina de estados" sencilla.
//    Cada pasoBarrido milisegundos se avanza incrementoBarrido grados.
// ---------------------------------------------------------------------------
bool barridoActivo = false;
int  direccionBarrido = +1;               // +1 sube, -1 baja
const int incrementoBarrido = 5;          // grados por paso
const unsigned long pasoBarrido = 40;     // milisegundos entre pasos
unsigned long marcaTiempoBarrido = 0;

// ---------------------------------------------------------------------------
// Recorta un angulo al rango valido 0..180
// ---------------------------------------------------------------------------
int recortarAngulo(int angulo) {
  if (angulo < anguloMinimo) return anguloMinimo;
  if (angulo > anguloMaximo) return anguloMaximo;
  return angulo;
}

// ---------------------------------------------------------------------------
// Mueve el servo y recuerda la posicion
// ---------------------------------------------------------------------------
void aplicarAngulo(int angulo) {
  anguloActual = recortarAngulo(angulo);
  servo.write(anguloActual);    // la libreria convierte grados -> ancho de pulso
}

// ---------------------------------------------------------------------------
// Confirma el angulo aplicado a TODOS los clientes (broadcast), para que todas
// las interfaces conectadas muestren la misma posicion real del servo.
// ---------------------------------------------------------------------------
void avisarAngulo() {
  JsonDocument documento;
  documento["tipo"]   = "servo_ack";
  documento["angulo"] = anguloActual;

  String texto;
  serializeJson(documento, texto);
  servidorSocket.broadcastTXT(texto);
}

// ---------------------------------------------------------------------------
// Responde un error SOLO al cliente que mando el comando equivocado
// ---------------------------------------------------------------------------
void enviarError(uint8_t numCliente, const char* mensaje) {
  JsonDocument documento;
  documento["tipo"]    = "error";
  documento["mensaje"] = mensaje;

  String texto;
  serializeJson(documento, texto);
  servidorSocket.sendTXT(numCliente, texto);
  Serial.print("Error: ");
  Serial.println(mensaje);
}

// ---------------------------------------------------------------------------
// Interpreta un comando JSON recibido desde la interfaz de Qt
// ---------------------------------------------------------------------------
void procesarComando(uint8_t numCliente, uint8_t* carga, size_t longitud) {
  JsonDocument documento;
  DeserializationError error = deserializeJson(documento, carga, longitud);
  if (error) {
    enviarError(numCliente, "JSON invalido");
    return;
  }

  const char* tipoMsg = documento["tipo"];
  if (tipoMsg == nullptr) {
    enviarError(numCliente, "Falta el campo \"tipo\"");
    return;
  }

  if (strcmp(tipoMsg, "servo") == 0) {
    // El campo "angulo" debe existir y ser un numero (entero o decimal).
    JsonVariant campoAngulo = documento["angulo"];
    if (!campoAngulo.is<int>() && !campoAngulo.is<float>()) {
      enviarError(numCliente, "Falta el campo numerico \"angulo\" (0..180)");
      return;
    }
    int pedido = (int)lround(campoAngulo.as<float>());
    barridoActivo = false;            // un comando manual cancela el barrido
    aplicarAngulo(pedido);            // recorta a 0..180 y mueve el servo
    if (pedido != anguloActual) {
      Serial.printf("Angulo %d fuera de rango, se aplico %d\n", pedido, anguloActual);
    }
    avisarAngulo();                   // confirmamos el angulo REAL aplicado
  }
  else if (strcmp(tipoMsg, "barrido") == 0) {
    barridoActivo = true;
    direccionBarrido = +1;
    aplicarAngulo(anguloMinimo);      // el barrido arranca en 0 grados
    marcaTiempoBarrido = millis();
    avisarAngulo();
    Serial.println("Barrido iniciado");
  }
  else if (strcmp(tipoMsg, "consulta") == 0) {
    avisarAngulo();
  }
  else {
    enviarError(numCliente, "Tipo de comando desconocido");
  }
}

// ---------------------------------------------------------------------------
// Callback del servidor WebSocket (se registra con servidorSocket.onEvent)
// ---------------------------------------------------------------------------
void alEventoWebSocket(uint8_t numCliente, WStype_t tipo, uint8_t* carga, size_t longitud) {
  switch (tipo) {
    case WStype_CONNECTED: {
      IPAddress ip = servidorSocket.remoteIP(numCliente);
      Serial.print("Cliente conectado: ");
      Serial.println(ip.toString());
      // saludo inicial con la posicion actual del servo
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
    case WStype_DISCONNECTED:
      Serial.printf("Cliente %u desconectado\n", numCliente);
      break;
    case WStype_TEXT:
      procesarComando(numCliente, carga, longitud);
      break;
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Avanza el barrido de demostracion (sin bloquear: usa millis(), no delay())
// ---------------------------------------------------------------------------
void actualizarBarrido() {
  if (!barridoActivo) return;
  if (millis() - marcaTiempoBarrido < pasoBarrido) return;
  marcaTiempoBarrido = millis();

  int siguiente = anguloActual + direccionBarrido * incrementoBarrido;
  if (siguiente >= anguloMaximo) {
    siguiente = anguloMaximo;
    direccionBarrido = -1;            // llegamos a 180: ahora regresamos
  }
  if (siguiente <= anguloMinimo && direccionBarrido < 0) {
    siguiente = anguloMinimo;
    barridoActivo = false;            // de vuelta en 0: termina el barrido
    Serial.println("Barrido terminado");
  }
  aplicarAngulo(siguiente);
  avisarAngulo();                     // la interfaz ve moverse el angulo
}

// ---------------------------------------------------------------------------
// Lee el potenciometro cada periodoPot ms y lo manda a TODOS los clientes
// (sin bloquear: usa millis(), igual que el barrido)
// ---------------------------------------------------------------------------
void enviarPotenciometro() {
  if (millis() - marcaTiempoPot < periodoPot) return;
  marcaTiempoPot = millis();
  if (servidorSocket.connectedClients() == 0) return;   // nadie escucha: no se manda

  int lectura = analogRead(pinPot);                     // 0..4095
  float voltaje = lectura * 3.3 / 4095.0;

  JsonDocument documento;
  documento["tipo"]    = "pot";
  documento["valor"]   = lectura;
  documento["voltaje"] = round(voltaje * 100) / 100.0;  // dos decimales

  String texto;
  serializeJson(documento, texto);
  servidorSocket.broadcastTXT(texto);
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Entrada analogica del potenciometro: 12 bits y rango completo de 0 a 3.3 V
  analogReadResolution(12);
  analogSetPinAttenuation(pinPot, ADC_11db);

  // Configurar el servo: 50 Hz y anchos de pulso del SG90
  ESP32PWM::allocateTimer(0);         // reserva un temporizador para el PWM
  servo.setPeriodHertz(50);           // un pulso cada 20 ms
  servo.attach(pinServo, pulsoMinimo, pulsoMaximo);
  aplicarAngulo(anguloActual);        // arranca al centro (90 grados)

  // Conectar al Access Point
  WiFi.begin(nombreRed, claveRed);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado. IP de la ESP32: ");
  Serial.println(WiFi.localIP());   // <-- esta IP se escribe en la interfaz de Qt

  // Iniciar el servidor WebSocket
  servidorSocket.begin();
  servidorSocket.onEvent(alEventoWebSocket);
  Serial.println("Servidor WebSocket iniciado en el puerto 81");
}

// ---------------------------------------------------------------------------
void loop() {
  servidorSocket.loop();   // atiende a los clientes WebSocket (no bloquea)
  actualizarBarrido();     // avanza el barrido si esta activo
  enviarPotenciometro();   // manda el potenciometro cada 100 ms
}
