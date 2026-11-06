#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ====== LIBRERÍAS OLED ======
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====== CONFIG OLED ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== CONFIG WiFi ======
const char* ssid     = "IRIDIA'S PHONE";
const char* password = "tumamaxd";
WebSocketsServer servidorWS(81);

// ====== PINES ======
#define PIN_TRIG 12
#define PIN_ECHO 13
#define STBY 10
#define PWMA 4  
#define AIN1 5  
#define AIN2 6  
#define PWMB 7  
#define BIN1 15 
#define BIN2 16 
#define PIN_SERVO 14

// ====== CONFIG SERVO Y MOTORES ======
const int pulsoMinimo = 500;  
const int pulsoMaximo = 2400; 
const int dutyMinimo = (pulsoMinimo * 16383) / 20000; 
const int dutyMaximo = (pulsoMaximo * 16383) / 20000; 
int anguloActual = 90;

const int VELOCIDAD = 130; // PWM (0-255) para controlar la velocidad de los motores

// ====== ESTADO ROOMBA Y VARIABLES COMPARTIDAS ======
bool modoRoombaActivo = false;
unsigned long ultimoMsUS = 0;

// Estas variables evitan que el loop() y RoombaTask() choquen al leer el sensor simultáneamente
volatile float distanciaGlobal = 0.0;
volatile bool lecturaOkGlobal = false;

void aplicarAngulo(int angulo) {
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;
  anguloActual = angulo;
  int duty = map(angulo, 0, 180, dutyMinimo, dutyMaximo);
  ledcWrite(PIN_SERVO, duty);
}

// ======================================================================
//                 MOTORES Y MOVIMIENTO (PWM)
// ======================================================================
static inline void motores_init_pines() {
  pinMode(STBY, OUTPUT);
  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  digitalWrite(STBY, HIGH);
}

static inline void motores_parar_total() { 
  analogWrite(PWMA, 0); digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  analogWrite(PWMB, 0); digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW); 
}
static inline void mover_avanzar() { 
  analogWrite(PWMA, VELOCIDAD); digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); 
  analogWrite(PWMB, VELOCIDAD); digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); 
}
static inline void mover_retroceder() { 
  analogWrite(PWMA, VELOCIDAD); digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); 
  analogWrite(PWMB, VELOCIDAD); digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); 
}
static inline void mover_izquierda() { 
  analogWrite(PWMA, VELOCIDAD); digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); 
  analogWrite(PWMB, VELOCIDAD); digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); 
} 
static inline void mover_derecha() { 
  analogWrite(PWMA, VELOCIDAD); digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); 
  analogWrite(PWMB, VELOCIDAD); digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); 
} 

enum class DirMov : uint8_t { Parado=0, Avanzar, Retroceder, Izquierda, Derecha, Parar };
struct CmdMotor { DirMov accion; uint32_t dur_ms; uint32_t t0_ms; };
static QueueHandle_t motorCmdQ = nullptr;

void MotorTask(void* arg) {
  (void)arg;
  bool enMovimiento = false;
  uint32_t t_fin = 0;
  for (;;) {
    CmdMotor nuevo;
    if (xQueueReceive(motorCmdQ, &nuevo, (TickType_t)0) == pdTRUE) {
      motores_parar_total();
      
      if (nuevo.accion == DirMov::Parar) {
        enMovimiento = false;
      } else {
        switch (nuevo.accion) {
          case DirMov::Avanzar:     mover_avanzar(); break;
          case DirMov::Retroceder:  mover_retroceder(); break;
          case DirMov::Izquierda:   mover_izquierda(); break;
          case DirMov::Derecha:     mover_derecha(); break;
          default: break;
        }
        
        if (nuevo.dur_ms > 0) {
          t_fin = millis() + nuevo.dur_ms;
          enMovimiento = true; 
        } else {
          enMovimiento = false; // Movimiento continuo sin timeout
        }
      }
    }
    
    // Verificación de timeout para detener el motor
    if (enMovimiento && (int32_t)(millis() - t_fin) >= 0) {
      enMovimiento = false;
      motores_parar_total();
    }
    vTaskDelay(1); 
  }
}

// ======================================================================
//                 LECTURA HC-SR04
// ======================================================================
float leerUltrasonico(bool &ok) {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duracion = pulseIn(PIN_ECHO, HIGH, 30000); 
  if (duracion > 0) {
    float dist = (duracion * 0.0343) / 2.0;
    if (dist >= 2.0 && dist <= 400.0) { ok = true; return dist; }
  }
  ok = false; return 0.0;
}

// ======================================================================
//                 LÓGICA ROOMBA (RADAR ACTIVO)
// ======================================================================
void RoombaTask(void* arg) {
  static bool roombaAvanzando = false; 

  for(;;) {
    if(modoRoombaActivo) {
      bool ok;
      float dist = leerUltrasonico(ok);
      
      // Actualizar globales para que Qt y OLED puedan leerlas sin usar el pin
      distanciaGlobal = dist;
      lecturaOkGlobal = ok;

      if(ok && dist < 25.0) { // OBSTÁCULO DETECTADO
        roombaAvanzando = false;

        // 1. FRENAR (Tiempo para detener inercia y bajar consumo)
        CmdMotor stopCmd{DirMov::Parar, 0, 0};
        xQueueSend(motorCmdQ, &stopCmd, 0);
        vTaskDelay(pdMS_TO_TICKS(200)); 

        // 2. EVASIÓN: Retroceder si está muy cerca
        if (dist < 10.0) {
          CmdMotor backCmd{DirMov::Retroceder, 500, 0};
          xQueueSend(motorCmdQ, &backCmd, 0);
          // Esperamos a que la reversa TERMINE por completo antes de mover el servo
          vTaskDelay(pdMS_TO_TICKS(650)); 
        }

        // 3. Radar Derecho
        aplicarAngulo(30);
        vTaskDelay(pdMS_TO_TICKS(500));
        float distDer = leerUltrasonico(ok);
        if(!ok) distDer = 999.0; // Si no hay eco, el camino está libre

        // 4. Radar Izquierdo (FIX DE CORRIENTE/BROWNOUT)
        // El salto de 30° a 150° exige corriente máxima. 
        // Hacemos una breve parada en 90° para romper el pico de consumo.
        aplicarAngulo(90);
        vTaskDelay(pdMS_TO_TICKS(200)); 
        aplicarAngulo(150);
        vTaskDelay(pdMS_TO_TICKS(400));
        
        float distIzq = leerUltrasonico(ok);
        if(!ok) distIzq = 999.0; 

        // 5. Decidir hacia dónde girar
        aplicarAngulo(90); 
        vTaskDelay(pdMS_TO_TICKS(300)); // Esperar que el cuello se centre antes de arrancar llantas
        
        if(distDer > distIzq) {
          CmdMotor giroCmd{DirMov::Derecha, 600, 0}; 
          xQueueSend(motorCmdQ, &giroCmd, 0);
        } else {
          CmdMotor giroCmd{DirMov::Izquierda, 600, 0}; 
          xQueueSend(motorCmdQ, &giroCmd, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(700)); 
        
      } else {
        // CAMINO LIBRE
        if (!roombaAvanzando) {
          aplicarAngulo(90); 
          vTaskDelay(pdMS_TO_TICKS(200));
          // Enviar comando continuo UNA SOLA VEZ
          CmdMotor fwdCmd{DirMov::Avanzar, 0, 0}; 
          xQueueSend(motorCmdQ, &fwdCmd, 0);
          roombaAvanzando = true;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 
      }
    } else {
      roombaAvanzando = false;
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

// ======================================================================
//                 EVENTOS WEBSOCKET
// ======================================================================
void onEventoWS(uint8_t idCliente, WStype_t tipo, uint8_t * payload, size_t longitud) {
  if (tipo == WStype_TEXT) {
    JsonDocument doc;
    if (deserializeJson(doc, payload, longitud)) return;
    
    if (doc["tipo"] == "mover" && !modoRoombaActivo) { 
      DirMov a = DirMov::Parar;
      if (doc["accion"] == "avanzar") a = DirMov::Avanzar;
      else if (doc["accion"] == "retroceder") a = DirMov::Retroceder;
      else if (doc["accion"] == "izquierda") a = DirMov::Izquierda;
      else if (doc["accion"] == "derecha") a = DirMov::Derecha;
      
      uint32_t ms_cmd = doc["ms"] | 0;
      CmdMotor cmd{ a, ms_cmd, 0 };
      xQueueSend(motorCmdQ, &cmd, 0);
    } 
    else if (doc["tipo"] == "servo" && !modoRoombaActivo) {
      int anguloDeseado = doc["angulo"];
      aplicarAngulo(anguloDeseado);
    }
    else if (doc["tipo"] == "roomba") {
      modoRoombaActivo = doc["activo"];
      if(!modoRoombaActivo) { 
        CmdMotor cmd{DirMov::Parar, 0, 0};
        xQueueSend(motorCmdQ, &cmd, 0);
        aplicarAngulo(90); 
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);

  // Configurar Servo
  ledcAttach(PIN_SERVO, 50, 14);
  aplicarAngulo(90);
  
  // Iniciar OLED I2C (SDA=8, SCL=9)
  Wire.begin(8, 9);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Fallo al iniciar OLED"));
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.print("Iniciando...");
    display.display();
  }

  // Iniciar WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP de la ESP32: ");
  Serial.println(WiFi.localIP());

  motores_init_pines();
  motores_parar_total();

  motorCmdQ = xQueueCreate(10, sizeof(CmdMotor));
  xTaskCreatePinnedToCore(MotorTask, "MotorTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(RoombaTask, "RoombaTask", 4096, nullptr, 1, nullptr, 1);

  servidorWS.begin();
  servidorWS.onEvent(onEventoWS);
  Serial.println("Servidor WebSocket iniciado en el puerto 81");
}

void loop() {
  servidorWS.loop();
  
  unsigned long ahora = millis();
  if (ahora - ultimoMsUS >= 100) {
    ultimoMsUS = ahora;
    bool ok_us = false;
    float dist = 0.0;
    
    // Leer sensor solo si Roomba NO está activo
    if (!modoRoombaActivo) {
        dist = leerUltrasonico(ok_us);
        distanciaGlobal = dist;
        lecturaOkGlobal = ok_us;
    } else {
        dist = distanciaGlobal;
        ok_us = lecturaOkGlobal;
    }
    
    // --- ACTUALIZACIÓN DE PANTALLA OLED ---
    display.clearDisplay();
    display.setCursor(10, 25);
    
    if (ok_us) {
      display.print(dist, 1); 
      display.print(" cm");
      
      if (dist < 20.0) {
        display.invertDisplay(true); 
      } else {
        display.invertDisplay(false); 
      }
    } else {
      display.print("Fuera de");
      display.setCursor(10, 45);
      display.print("rango");
      display.invertDisplay(false);
    }
    display.display();
    // --------------------------------------

    // --- TELEMETRÍA WEBSOCKET ---
    JsonDocument docUS;
    docUS["tipo"] = "ultrasonico";
    docUS["ok"] = ok_us;
    docUS["dist"] = ok_us ? (round(dist * 10) / 10.0) : 0.0;
    
    String jsonWS;
    serializeJson(docUS, jsonWS);
    servidorWS.broadcastTXT(jsonWS);
  }
}
