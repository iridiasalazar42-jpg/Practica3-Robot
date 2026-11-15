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
        ui->botonConectar->setText("Conectar");
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

    if (obj["tipo"].toString() == "ultrasonico" && obj["ok"].toBool() == true) {
        double distanciaCruda = obj["dist"].toDouble();

        if (distanciaCruda >= 2.0 && distanciaCruda <= 400.0) {
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
    socket.sendTextMessage(doc.toJson(QJsonDocument::Compact));

    if (!checked) {
        ui->sliderServo->setValue(90);
    }
}

// ==========================================================
//                 CONTROL MANUAL DEL ROBOT
// ==========================================================

void MainWindow::enviarComandoManual(QString accion)
{
    if (socket.state() != QAbstractSocket::ConnectedState) return;
    if (ui->checkRoomba->isChecked()) return;

    QJsonObject comando;
    comando["tipo"] = "mover";
    comando["accion"] = accion;
    comando["ms"] = 0; // 0 significa movimiento continuo

    QJsonDocument doc(comando);
    socket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWindow::detenerRobot()
{
    enviarComandoManual("parar");
}

// Iniciar movimiento al PRESIONAR
void MainWindow::on_pushButtonW_pressed() { enviarComandoManual("avanzar"); }
void MainWindow::on_pushButtonS_pressed() { enviarComandoManual("retroceder"); }
void MainWindow::on_pushButtonA_pressed() { enviarComandoManual("derecha"); } // Dependiendo de tu cableado, A suele ser izquierda
void MainWindow::on_pushButtonD_pressed() { enviarComandoManual("izquierda"); }

// Detener movimiento al SOLTAR
void MainWindow::on_pushButtonW_released() { detenerRobot(); }
void MainWindow::on_pushButtonS_released() { detenerRobot(); }
void MainWindow::on_pushButtonA_released() { detenerRobot(); }
void MainWindow::on_pushButtonD_released() { detenerRobot(); }

// Paro de emergencia o espacio
void MainWindow::on_pushButtonSpace_clicked() { detenerRobot(); }

void MainWindow::on_sliderServo_valueChanged(int value)
{
    if (socket.state() != QAbstractSocket::ConnectedState) return;
    if (ui->checkRoomba->isChecked()) return;

    QJsonObject comando;
    comando["tipo"] = "servo";
    comando["angulo"] = value;

    QJsonDocument doc(comando);
    socket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

// Captura de teclado (útil si pruebas en PC antes de pasar a Android)
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_W: on_pushButtonW_pressed(); ui->pushButtonW->setDown(true); break;
    case Qt::Key_S: on_pushButtonS_pressed(); ui->pushButtonS->setDown(true); break;
    case Qt::Key_A: on_pushButtonA_pressed(); ui->pushButtonA->setDown(true); break;
    case Qt::Key_D: on_pushButtonD_pressed(); ui->pushButtonD->setDown(true); break;
    case Qt::Key_Space: on_pushButtonSpace_clicked(); ui->pushButtonSpace->setDown(true); break;
    default: QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_W: on_pushButtonW_released(); ui->pushButtonW->setDown(false); break;
    case Qt::Key_S: on_pushButtonS_released(); ui->pushButtonS->setDown(false); break;
    case Qt::Key_A: on_pushButtonA_released(); ui->pushButtonA->setDown(false); break;
    case Qt::Key_D: on_pushButtonD_released(); ui->pushButtonD->setDown(false); break;
    case Qt::Key_Space: ui->pushButtonSpace->setDown(false); break;
    default: QMainWindow::keyReleaseEvent(event);
    }
}