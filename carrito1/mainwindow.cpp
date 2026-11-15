#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    setFocusPolicy(Qt::StrongFocus);


    connect(&m_webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MainWindow::onDisconnected);



    connect(ui->btnW, &QPushButton::clicked, this, [=](){ enviarComando("avanzar"); });
    connect(ui->btnS, &QPushButton::clicked, this, [=](){ enviarComando("retroceder"); });
    connect(ui->btnA, &QPushButton::clicked, this, [=](){ enviarComando("izquierda"); });
    connect(ui->btnD, &QPushButton::clicked, this, [=](){ enviarComando("derecha"); });
    connect(ui->btnSTOP, &QPushButton::clicked, this, [=](){ enviarComando("parar"); });

}

MainWindow::~MainWindow()
{
    delete ui;
}


// --- WebSocket Callbacks ---
void MainWindow::onConnected()
{
    qDebug() << "Conectado al servidor ESP32!";
    ui->pushButton->setText("Desconectar");

    // Devolver el foco a la ventana principal.
    // Esto es crucial porque al hacer clic en el botón, el botón "roba" el foco del teclado,
    // lo que haría que tus teclas WASD dejen de responder hasta que hagas clic en el fondo.
    this->setFocus();
}

void MainWindow::onDisconnected()
{
    qDebug() << "Desconectado del servidor ESP32.";
    ui->pushButton->setText("Conectar xd");
}

// --- Constructor del JSON ---
void MainWindow::enviarComando(QString accion)
{
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        QJsonObject json;
        json["tipo"] = "mover";
        json["accion"] = accion;
        json["ms"] = 500; // Tiempo en milisegundos que el motor girará

        QJsonDocument doc(json);
        m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));

        qDebug() << "Comando enviado:" << accion;
    } else {
        qDebug() << "Error: No estás conectado al ESP32.";
    }
}

// --- Controles de Teclado (WASD) ---
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Ignorar si la tecla se mantiene presionada (auto-repeat) para no saturar al ESP32
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_W: enviarComando("avanzar"); break;
    case Qt::Key_S: enviarComando("retroceder"); break;
    case Qt::Key_A: enviarComando("izquierda"); break;
    case Qt::Key_D: enviarComando("derecha"); break;
    case Qt::Key_Space: enviarComando("parar"); break;
    default: QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    // Solo enviar "parar" si se soltó una tecla de movimiento
    switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_S:
    case Qt::Key_A:
    case Qt::Key_D:
        enviarComando("parar");
        break;
    default:
        break;
    }

    QMainWindow::keyReleaseEvent(event);
}



void MainWindow::on_pushButton_clicked()
{
    // Si no está conectado, intentar conectar
    if (m_webSocket.state() == QAbstractSocket::UnconnectedState) {
        ui->pushButton->setText("Conectando...");
        m_webSocket.open(QUrl(url));
    }
    // Si ya está conectado (o conectando), desconectar
    else {
        m_webSocket.close();
    }
}

