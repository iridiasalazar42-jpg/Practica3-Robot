# ============================================================================
#  Ejemplo 1 (Clase 10) - "Espejo del boton" (WebSocket) - Interfaces (I7262)
#  App de Qt que empareja con 01_boton_retencion_led por WebSocket:
#  se conecta a ws://<IP>:81 y refleja el LED en tiempo real ("LED:ON"/"LED:OFF").
# ============================================================================
QT += core gui widgets websockets
CONFIG += c++17

SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h
