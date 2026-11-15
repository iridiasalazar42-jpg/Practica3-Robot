#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Configurar la gráfica
    grafica->addSeries(serie);
    grafica->legend()->hide();
    grafica->setTitle("Distancia HC-SR04 por WiFi (Media Móvil)");

    ejeTiempo->setTitleText("Tiempo (s)");
    ejeTiempo->setRange(0, 10);
    ejeDistancia->setTitleText("Distancia (cm)");
    ejeDistancia->setRange(0, 100);

    grafica->addAxis(ejeTiempo, Qt::AlignBottom);
    grafica->addAxis(ejeDistancia, Qt::AlignLeft);
    serie->attachAxis(ejeTiempo);
    serie->attachAxis(ejeDistancia);

    ui->graficaDistancia->setChart(grafica);
    ui->graficaDistancia->setRenderHint(QPainter::Antialiasing);

    // Configurar barra de proximidad
    ui->barraProximidad->setRange(0, 400);
    ui->barraProximidad->setValue(0);
    ui->barraProximidad->setTextVisible(false);

    // Conectar señales del WebSocket
    connect(&socket, &QWebSocket::connected, this, [this]() {
        ui->botonConectar->setText("Desconectar");
        serie->clear();
        bufferFiltro.clear();
        reloj.start();
    });

    connect(&socket, &QWebSocket::disconnected, this, [this]() {
        ui->botonConectar->setText("Conectar xd");
    });

    connect(&socket, &QWebSocket::textMessageReceived, this, &MainWindow::onMensajeRecibido);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_botonConectar_clicked()
{
    if (socket.state() == QAbstractSocket::ConnectedState) {
        socket.close();
    } else {
        QString ip = ui->lineEditIp->text();
        socket.open(QUrl("ws://" + ip + ":81"));
    }
}

double MainWindow::aplicarFiltroMediaMovil(double nuevaDistancia)
{
    bufferFiltro.append(nuevaDistancia);
    if (bufferFiltro.size() > 5) {
        bufferFiltro.removeFirst();
    }
    double suma = 0;
    for (double d : bufferFiltro) suma += d;
    return suma / bufferFiltro.size();
}

void MainWindow::onMensajeRecibido(const QString &mensaje)
{
    QJsonDocument doc = QJsonDocument::fromJson(mensaje.toUtf8());
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();

    // Solo procesar mensajes del sensor ultrasónico
    if (obj["tipo"].toString() == "ultrasonico" && obj["ok"].toBool() == true) {
        double distanciaCruda = obj["dist"].toDouble();

        // Filtro 1: Rango válido
        if (distanciaCruda >= 2.0 && distanciaCruda <= 400.0) {

            // Filtro 2: Media Móvil
            double distanciaFiltrada = aplicarFiltroMediaMovil(distanciaCruda);

            ui->etiquetaDistancia->setText(QString::number(distanciaFiltrada, 'f', 1) + " cm");

            ui->barraProximidad->setValue((int)distanciaFiltrada);
            if (distanciaFiltrada < 20.0) {
                ui->barraProximidad->setStyleSheet("QProgressBar::chunk { background-color: red; }");
            } else {
                ui->barraProximidad->setStyleSheet("QProgressBar::chunk { background-color: #0078d7; }");
            }

            double t = reloj.elapsed() / 1000.0;
            serie->append(t, distanciaFiltrada);

            if (t > 10) ejeTiempo->setRange(t - 10, t);
            if (serie->count() > 200) serie->remove(0);
        }
    }
}
void MainWindow::on_checkRoomba_toggled(bool checked)
{
    QJsonObject comando;
    comando["tipo"] = "roomba";
    comando["activo"] = checked;

    QJsonDocument doc(comando);
    QString texto = doc.toJson(QJsonDocument::Compact);
    socket.sendTextMessage(texto);

    if (!checked) {
        ui->sliderServo->setValue(90);
    }
}

// ==========================================================
//                 CONTROL MANUAL DEL ROBOT
// ==========================================================

void MainWindow::enviarComandoManual(QString accion)
{
    // Solo enviar si hay conexión y si el modo Roomba está apagado
    if (socket.state() != QAbstractSocket::ConnectedState) return;
    if (ui->checkRoomba->isChecked()) return; // Evita pelear con la IA del robot

    QJsonObject comando;
    comando["tipo"] = "mover";
    comando["accion"] = accion;
    comando["ms"] = 600; // Duración del pulso de movimiento en ms

    QJsonDocument doc(comando);
    socket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

// Conexión de los clics en la UI
void MainWindow::on_pushButtonW_clicked() { enviarComandoManual("avanzar"); }
void MainWindow::on_pushButtonS_clicked() { enviarComandoManual("retroceder"); }
void MainWindow::on_pushButtonA_clicked() { enviarComandoManual("derecha"); }
void MainWindow::on_pushButtonD_clicked() { enviarComandoManual("izquierda"); }
void MainWindow::on_pushButtonSpace_clicked() { enviarComandoManual("parar"); }

// Captura del teclado
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Evitar que el comando se envíe repetidamente si mantienes la tecla presionada
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_W:
        ui->pushButtonW->click(); // Simula el clic visual en el botón
        break;
    case Qt::Key_S:
        ui->pushButtonS->click();
        break;
    case Qt::Key_A:
        ui->pushButtonA->click();
        break;
    case Qt::Key_D:
        ui->pushButtonD->click();
        break;
    case Qt::Key_Space:
        ui->pushButtonSpace->click();
        break;
    default:
        // Si es otra tecla, que la ventana la maneje normalmente
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::on_sliderServo_valueChanged(int value)
{
    // Solo enviar si hay conexión
    if (socket.state() != QAbstractSocket::ConnectedState) return;

    // Evitar moverlo manualmente si el modo Roomba está controlando el cuello
    if (ui->checkRoomba->isChecked()) return;

    QJsonObject comando;
    comando["tipo"] = "servo";
    comando["angulo"] = value;

    QJsonDocument doc(comando);
    socket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}