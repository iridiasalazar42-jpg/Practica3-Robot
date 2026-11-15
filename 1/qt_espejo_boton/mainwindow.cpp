#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Ejemplo 1 - Espejo del boton (WebSocket + JSON)");

    QWidget *central = new QWidget(this);
    QVBoxLayout *raiz = new QVBoxLayout(central);

    QHBoxLayout *fila = new QHBoxLayout();
    fila->addWidget(new QLabel("IP de la ESP32:", central));
    campoIp = new QLineEdit("192.168.0.50", central);
    botonConectar = new QPushButton("Conectar", central);
    fila->addWidget(campoIp);
    fila->addWidget(botonConectar);
    raiz->addLayout(fila);

    indicador = new QLabel("APAGADO", central);
    indicador->setAlignment(Qt::AlignCenter);
    indicador->setStyleSheet("font-size:36px; font-weight:bold; padding:40px;"
                             "background:#dddddd; border-radius:12px;");
    raiz->addWidget(indicador);

    etiquetaAyuda = new QLabel("Escribe la IP y conecta. Luego presiona el boton fisico "
                               "de la ESP32: la ventana reflejara el LED (JSON por WebSocket).", central);
    etiquetaAyuda->setWordWrap(true);
    raiz->addWidget(etiquetaAyuda);

    setCentralWidget(central);
    resize(470, 300);

    connect(botonConectar, &QPushButton::clicked, this, &MainWindow::alConectar);
    connect(&socket, &QWebSocket::connected, this, &MainWindow::alConectarSocket);
    connect(&socket, &QWebSocket::disconnected, this, &MainWindow::alDesconectarSocket);
    connect(&socket, &QWebSocket::textMessageReceived, this, &MainWindow::alRecibirMensaje);
}

void MainWindow::alConectar() {
    const QString ip = campoIp->text().trimmed();
    if (ip.isEmpty())
        return;
    socket.open(QUrl("ws://" + ip + ":81"));
    etiquetaAyuda->setText("Conectando a ws://" + ip + ":81 ...");
}

void MainWindow::alConectarSocket() {
    etiquetaAyuda->setText("Conectado. Presiona el boton fisico de la ESP32.");
}

void MainWindow::alDesconectarSocket() {
    etiquetaAyuda->setText("Desconectado. Verifica la IP y la red GWN571D04.");
}

void MainWindow::alRecibirMensaje(const QString &mensaje) {
    const QJsonObject obj = QJsonDocument::fromJson(mensaje.toUtf8()).object();
    if (obj.value("tipo").toString() == "estado_led")
        mostrarEstado(obj.value("encendido").toBool());
}

void MainWindow::mostrarEstado(bool encendido) {
    indicador->setText(encendido ? "ENCENDIDO" : "APAGADO");
    indicador->setStyleSheet(
        QString("font-size:36px; font-weight:bold; padding:40px; border-radius:12px;"
                "background:%1; color:%2;")
            .arg(encendido ? "#2e7d32" : "#dddddd")
            .arg(encendido ? "white" : "black"));
}
