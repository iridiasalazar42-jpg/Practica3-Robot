#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->grupoServo->setEnabled(false);     // sin conexion no se puede mover el servo
    ui->registro->setReadOnly(true);

    // ---- Señales del WebSocket: connect + lambdas ----
    connect(&socket, &QWebSocket::connected, this, [this]() {
        serie->clear();                    // la grafica empieza de cero en cada conexion
        reloj.start();
        ui->etiquetaEstado->setText("Conectado");
        ui->botonConectar->setText("Desconectar");
        ui->grupoServo->setEnabled(true);
        registrar("Conectado a la ESP32.");
    });

    connect(&socket, &QWebSocket::disconnected, this, [this]() {
        ui->etiquetaEstado->setText("Desconectado");
        ui->botonConectar->setText("Conectar");
        ui->grupoServo->setEnabled(false);
        registrar("Desconectado.");
    });

    connect(&socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        registrar("Error: " + socket.errorString());
    });

    connect(&socket, &QWebSocket::textMessageReceived, this, [this](const QString &mensaje) {
        QJsonObject objeto = QJsonDocument::fromJson(mensaje.toUtf8()).object();
        QString tipo = objeto["tipo"].toString();
        if (tipo == "pot") {                // 10 lecturas por segundo: van a la grafica, no al registro
            agregarLectura(objeto["voltaje"].toDouble());
            return;
        }
        registrar("<- " + mensaje);
        if (tipo == "saludo" || tipo == "servo_ack") {
            int angulo = objeto["angulo"].toInt();
            ui->etiquetaConfirmado->setText(QString::number(angulo) + "°");
        }
    });

    // ---- Grafica del potenciometro (Qt Charts) ----
    grafica->addSeries(serie);
    grafica->legend()->hide();
    grafica->setTitle("Potenciometro en el GPIO 8");
    ejeTiempo->setTitleText("Tiempo (s)");
    ejeTiempo->setRange(0, 10);
    ejeVoltaje->setTitleText("Voltaje (V)");
    ejeVoltaje->setRange(0, 3.3);
    ejeVoltaje->setTickCount(4);            // marcas en 0, 1.1, 2.2 y 3.3 V
    grafica->addAxis(ejeTiempo, Qt::AlignBottom);
    grafica->addAxis(ejeVoltaje, Qt::AlignLeft);
    serie->attachAxis(ejeTiempo);
    serie->attachAxis(ejeVoltaje);
    ui->graficaPot->setChart(grafica);
    ui->graficaPot->setRenderHint(QPainter::Antialiasing);

    // ---- Limite de envio: el temporizador manda el angulo pendiente ----
    temporizador.setInterval(50);
    connect(&temporizador, &QTimer::timeout, this, [this]() {
        if (anguloPendiente < 0) {          // no hay nada nuevo: se detiene
            temporizador.stop();
            return;
        }
        QJsonObject comando;
        comando["tipo"] = "servo";
        comando["angulo"] = anguloPendiente;
        enviarJson(comando);
        anguloPendiente = -1;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ---- Slots creados con "Go to slot..." ----

void MainWindow::on_botonConectar_clicked()
{
    if (socket.state() == QAbstractSocket::ConnectedState) {
        socket.close();
        return;
    }
    QString url = "ws://" + ui->lineEditIp->text() + ":" + QString::number(ui->spinPuerto->value()) + "/";
    registrar("Conectando a " + url + " ...");
    ui->etiquetaEstado->setText("Conectando...");
    socket.open(QUrl(url));
}

void MainWindow::on_deslizadorAngulo_valueChanged(int value)
{
    ui->etiquetaPedido->setText(QString::number(value) + "°");
    anguloPendiente = value;
    if (!temporizador.isActive()) {
        temporizador.start();               // envia en el siguiente tick (<= 50 ms)
    }
}

void MainWindow::on_botonConsultar_clicked()
{
    QJsonObject comando;
    comando["tipo"] = "consulta";
    enviarJson(comando);
}

void MainWindow::on_botonBarrido_clicked()
{
    QJsonObject comando;
    comando["tipo"] = "barrido";
    enviarJson(comando);
}

// ---- Funciones auxiliares ----

void MainWindow::enviarJson(const QJsonObject &objeto)
{
    QString texto = QJsonDocument(objeto).toJson(QJsonDocument::Compact);
    socket.sendTextMessage(texto);
    registrar("-> " + texto);
}

void MainWindow::registrar(const QString &linea)
{
    QString hora = QTime::currentTime().toString("HH:mm:ss.zzz");
    ui->registro->appendPlainText(hora + "  " + linea);
}

void MainWindow::agregarLectura(double voltaje)
{
    double t = reloj.elapsed() / 1000.0;           // segundos desde que se conecto
    serie->append(t, voltaje);
    if (serie->count() > 200) {                    // guarda solo los ultimos 20 s
        serie->remove(0);
    }
    if (t > 10) {
        ejeTiempo->setRange(t - 10, t);            // ventana de 10 s que avanza
    }
}