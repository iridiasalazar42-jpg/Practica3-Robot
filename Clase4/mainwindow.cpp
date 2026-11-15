#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug> // Necesario para la Evidencia 2

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Conectar las señales del WebSocket (Asegúrate de tener estos slots declarados en tu mainwindow.h)
    connect(&socket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(&socket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Botón de Conectar (pushButton_2)
void MainWindow::on_pushButton_2_clicked()
{
    if(socket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Intentando conectar a ws://10.182.149.221:81...";
        socket.open(QUrl("ws://10.182.149.221:81"));
    }
}

void MainWindow::onConnected()
{
    qDebug() << "[Qt] Conectado al servidor WebSocket.";
}

// Función para procesar los mensajes que llegan de la ESP32
void MainWindow::onTextMessageReceived(QString message)
{
    qDebug() << "[Qt] RX <- ESP32:" << message;

    // Opcional para 100% de la rúbrica: Aquí podrías parsear el JSON entrante
    // {"tipo":"estado", "leds":[true,false...]} y actualizar los colores de
    // los botones basándote en la respuesta real de la ESP32.
}

// Función auxiliar para enviar y registrar en consola
void MainWindow::enviarJson(const QJsonObject &objeto)
{
    QJsonDocument documento(objeto);
    QString texto = QString::fromUtf8(documento.toJson(QJsonDocument::Compact));

    qDebug() << "[Qt] TX -> ESP32:" << texto; // Imprime en el Application Output para tu Evidencia 2
    socket.sendTextMessage(texto);
}

// Canal 4 - GPIO 13 (pushButton)
void MainWindow::on_pushButton_clicked(bool checked)
{
    if(socket.state() != QAbstractSocket::ConnectedState) return;

    QJsonObject obj;
    obj["tipo"] = "salida";
    obj["canal"] = 4; // Cambiado de "pin":13 a "canal":4 según el protocolo

    if(checked) {
        ui->pushButton->setStyleSheet(verdeEncencido);
        obj["encendido"] = true; // Cambiado de "estado":1 a "encendido":true
    } else {
        ui->pushButton->setStyleSheet(grisApagado);
        obj["encendido"] = false;
    }
    enviarJson(obj);
}

// Canal 3 - GPIO 12 (pushButton_3)
void MainWindow::on_pushButton_3_clicked(bool checked)
{
    if(socket.state() != QAbstractSocket::ConnectedState) return;

    QJsonObject obj1;
    obj1["tipo"] = "salida";
    obj1["canal"] = 3;

    if(checked) {
        ui->pushButton_3->setStyleSheet(verdeEncencido);
        obj1["encendido"] = true;
    } else {
        ui->pushButton_3->setStyleSheet(grisApagado);
        obj1["encendido"] = false;
    }
    enviarJson(obj1);
}

// Canal 2 - GPIO 11 (pushButton_4)
void MainWindow::on_pushButton_4_clicked(bool checked)
{
    if(socket.state() != QAbstractSocket::ConnectedState) return;

    QJsonObject obj2;
    obj2["tipo"] = "salida";
    obj2["canal"] = 2;

    if(checked) {
        ui->pushButton_4->setStyleSheet(verdeEncencido);
        obj2["encendido"] = true;
    } else {
        ui->pushButton_4->setStyleSheet(grisApagado);
        obj2["encendido"] = false;
    }
    enviarJson(obj2);
}

// Canal 1 - GPIO 10 (pushButton_5)
void MainWindow::on_pushButton_5_clicked(bool checked)
{
    if(socket.state() != QAbstractSocket::ConnectedState) return;

    QJsonObject obj3;
    obj3["tipo"] = "salida";
    obj3["canal"] = 1;

    if(checked) {
        ui->pushButton_5->setStyleSheet(verdeEncencido);
        obj3["encendido"] = true;
    } else {
        ui->pushButton_5->setStyleSheet(grisApagado);
        obj3["encendido"] = false;
    }
    enviarJson(obj3);
}