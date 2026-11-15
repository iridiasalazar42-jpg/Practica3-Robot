#include "mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ==========================================================
    // 1. CREACIÓN DE LAS PESTAÑAS
    // ==========================================================
    auto *pestanas = new QTabWidget(this);
    QWidget *pagRobot = new QWidget;
    QWidget *pagTemp = new QWidget;

    pestanas->addTab(pagRobot, "Robot");
    pestanas->addTab(pagTemp, "Temperatura");
    setCentralWidget(pestanas);

    // ==========================================================
    // 2. CONSTRUCCIÓN DE LA PESTAÑA "ROBOT" (Sin usar .ui)
    // ==========================================================
    QVBoxLayout *layoutRobot = new QVBoxLayout(pagRobot);

    QHBoxLayout *layoutTop = new QHBoxLayout;
    m_ip = new QLineEdit("192.168.");
    m_btnConectar = new QPushButton("Conectar");
    m_checkRoomba = new QCheckBox("Roomba");
    layoutTop->addWidget(m_ip);
    layoutTop->addWidget(m_btnConectar);
    layoutTop->addWidget(m_checkRoomba);
    layoutRobot->addLayout(layoutTop);

    QLabel *lblAlumnoRobot = new QLabel(ALUMNO);
    lblAlumnoRobot->setAlignment(Qt::AlignCenter);

    m_etiquetaDistancia = new QLabel("0.0 cm");
    QFont fontDist = m_etiquetaDistancia->font();
    fontDist.setPointSize(28); fontDist.setBold(true);
    m_etiquetaDistancia->setFont(fontDist);
    m_etiquetaDistancia->setAlignment(Qt::AlignCenter);

    m_barraProximidad = new QProgressBar;
    m_barraProximidad->setRange(0, 400);
    m_barraProximidad->setValue(0);
    m_barraProximidad->setTextVisible(false);

    layoutRobot->addWidget(lblAlumnoRobot);
    layoutRobot->addWidget(m_etiquetaDistancia);
    layoutRobot->addWidget(m_barraProximidad);

    grafica = new QChart;
    serie = new QLineSeries;
    ejeTiempo = new QValueAxis;
    ejeDistancia = new QValueAxis;

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

    m_graficaDistancia = new QChartView(grafica);
    m_graficaDistancia->setRenderHint(QPainter::Antialiasing);
    layoutRobot->addWidget(m_graficaDistancia);

    QGroupBox *groupBox = new QGroupBox("Panel de Control Robot");
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #aaa; border-radius: 8px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }");
    QVBoxLayout *layoutControls = new QVBoxLayout(groupBox);

    m_sliderServo = new QSlider(Qt::Horizontal);
    m_sliderServo->setRange(0, 180);
    m_sliderServo->setValue(90);
    layoutControls->addWidget(m_sliderServo);

    QGridLayout *gridBotones = new QGridLayout;
    m_btnW = new QPushButton("▲"); m_btnW->setMinimumHeight(60);
    m_btnA = new QPushButton("◀"); m_btnA->setMinimumHeight(60);
    m_btnD = new QPushButton("▶"); m_btnD->setMinimumHeight(60);
    m_btnS = new QPushButton("▼"); m_btnS->setMinimumHeight(60);
    m_btnSpace = new QPushButton("STOP"); m_btnSpace->setMinimumHeight(60);
    m_btnSpace->setStyleSheet("background-color: #ff4c4c; color: white; font-weight: bold; border-radius: 5px;");

    gridBotones->addWidget(m_btnW, 0, 1);
    gridBotones->addWidget(m_btnA, 1, 0);
    gridBotones->addWidget(m_btnSpace, 1, 1);
    gridBotones->addWidget(m_btnD, 1, 2);
    gridBotones->addWidget(m_btnS, 2, 1);
    layoutControls->addLayout(gridBotones);
    layoutRobot->addWidget(groupBox);

    m_registro = new QTextEdit;
    m_registro->setReadOnly(true);
    m_registro->setMaximumHeight(110);
    layoutRobot->addWidget(m_registro);

    // ==========================================================
    // 3. CONSTRUCCIÓN DE LA PESTAÑA "TEMPERATURA" (P4)
    // ==========================================================
    QVBoxLayout *layoutTemp = new QVBoxLayout(pagTemp);

    QLabel *lblAlumnoTemp = new QLabel(ALUMNO);
    lblAlumnoTemp->setStyleSheet("font-weight: bold; color: blue;");
    layoutTemp->addWidget(lblAlumnoTemp);

    // Botón y Etiqueta
    QPushButton *btnLeerTemp = new QPushButton("Leer temperatura");
    m_temperatura = new QLabel("-- °C");
    m_temperatura->setStyleSheet("font-size: 24px; font-weight: bold;");
    layoutTemp->addWidget(btnLeerTemp);
    layoutTemp->addWidget(m_temperatura);

    connect(btnLeerTemp, &QPushButton::clicked, this, [this]() {
        if (socket.state() == QAbstractSocket::ConnectedState) {
            socket.sendTextMessage("{\"tipo\":\"leer_dht\"}");
        } else {
            registrar("Error: No conectado");
        }
    });

    // Automático
    m_autoTemp = new QCheckBox("Guardar cada 10 s");
    layoutTemp->addWidget(m_autoTemp);
    m_timerTemp = new QTimer(this);

    connect(m_timerTemp, &QTimer::timeout, this, [this]() {
        if (socket.state() == QAbstractSocket::ConnectedState) {
            socket.sendTextMessage("{\"tipo\":\"leer_dht\"}");
        }
    });
    connect(m_autoTemp, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) m_timerTemp->start(10000);
        else m_timerTemp->stop();
    });

    // Tabla con 5 columnas (Agregada Humedad)
    m_tabla = new QTableWidget(0, 5);
    m_tabla->setHorizontalHeaderLabels({"id", "fecha_hora", "temperatura", "nota", "humedad"});
    m_tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layoutTemp->addWidget(m_tabla);

    m_total = new QLabel("Mediciones guardadas: 0");
    QPushButton *btnCargar = new QPushButton("Cargar historial");
    layoutTemp->addWidget(m_total);
    layoutTemp->addWidget(btnCargar);
    connect(btnCargar, &QPushButton::clicked, this, &MainWindow::cargarHistorial);

    // Notas (UPDATE)
    QHBoxLayout *layoutNota = new QHBoxLayout;
    m_id = new QSpinBox;
    m_id->setRange(0, 99999);
    m_id->setSpecialValueText("-");
    m_nota = new QLineEdit;
    m_nota->setPlaceholderText("Nota...");
    QPushButton *btnGuardarNota = new QPushButton("Guardar nota");

    layoutNota->addWidget(new QLabel("ID:"));
    layoutNota->addWidget(m_id);
    layoutNota->addWidget(m_nota);
    layoutNota->addWidget(btnGuardarNota);
    layoutTemp->addLayout(layoutNota);

    connect(m_tabla, &QTableWidget::cellClicked, this, [this](int fila, int) {
        m_id->setValue(m_tabla->item(fila, 0)->text().toInt());
    });

    connect(btnGuardarNota, &QPushButton::clicked, this, [this]() {
        if (m_id->value() == 0) {
            registrar("Primero toca una fila");
            return;
        }
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE mediciones SET nota = :nota WHERE id = :id");
        updateQuery.bindValue(":nota", m_nota->text());
        updateQuery.bindValue(":id", m_id->value());
        if (!updateQuery.exec()) {
            registrar("UPDATE falló: " + updateQuery.lastError().text());
        } else {
            registrar("Filas cambiadas: " + QString::number(updateQuery.numRowsAffected()));
            cargarHistorial();
        }
    });

    // Botón de Limpieza (DELETE)
    QPushButton *btnLimpiar = new QPushButton("Extra: Borrar historial antiguo (Dejar 10)");
    btnLimpiar->setStyleSheet("background-color: #ff9999; color: black; font-weight: bold;");
    layoutTemp->addWidget(btnLimpiar);

    connect(btnLimpiar, &QPushButton::clicked, this, [this]() {
        QSqlQuery deleteQuery;
        if (deleteQuery.exec("DELETE FROM mediciones WHERE id NOT IN "
                             "(SELECT id FROM mediciones ORDER BY id DESC LIMIT 10)")) {

            int borradas = deleteQuery.numRowsAffected();
            registrar("Se borraron " + QString::number(borradas) + " filas antiguas.");
            cargarHistorial();
        } else {
            registrar("Error al borrar el historial.");
        }
    });

    // ==========================================================
    // 4. CREACIÓN DE LA BASE DE DATOS (Con ALTER TABLE para Humedad)
    // ==========================================================
    const QString carpeta = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(carpeta);
    const QString ruta = carpeta + "/temperatura_3764.db";
    const bool yaExistia = QFile::exists(ruta);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(ruta);
    if (!db.open()) {
        registrar("No se abrió la base: " + db.lastError().text());
    } else {
        QSqlQuery consulta;
        if (!consulta.exec("CREATE TABLE IF NOT EXISTS mediciones ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "fecha_hora TEXT NOT NULL, "
                           "temperatura REAL NOT NULL, "
                           "fuente TEXT NOT NULL, "
                           "nota TEXT)")) {
            registrar("No se creó la tabla: " + consulta.lastError().text());
        } else {
            // ALTER TABLE para el Extra de Humedad
            QSqlQuery alterQuery;
            alterQuery.exec("ALTER TABLE mediciones ADD COLUMN humedad REAL");

            registrar((yaExistia ? "Base encontrada: " : "Base nueva creada: ") + ruta);
            cargarHistorial();
        }
    }

    // ==========================================================
    // 5. CONEXIONES DE SEÑALES
    // ==========================================================
    connect(m_btnConectar, &QPushButton::clicked, this, &MainWindow::on_botonConectar_clicked);
    connect(&socket, &QWebSocket::connected, this, [this]() {
        m_btnConectar->setText("Desconectar");
        serie->clear();
        bufferFiltro.clear();
        reloj.start();
        registrar("Conectado al robot");
    });
    connect(&socket, &QWebSocket::disconnected, this, [this]() {
        m_btnConectar->setText("Conectar");
        registrar("Desconectado");
        m_autoTemp->setChecked(false);
        m_timerTemp->stop();
    });
    connect(&socket, &QWebSocket::textMessageReceived, this, &MainWindow::onMensajeRecibido);

    connect(m_checkRoomba, &QCheckBox::toggled, this, [this](bool checked){
        QJsonObject comando;
        comando["tipo"] = "roomba";
        comando["activo"] = checked;
        socket.sendTextMessage(QJsonDocument(comando).toJson(QJsonDocument::Compact));
        if (!checked) m_sliderServo->setValue(90);
    });

    connect(m_sliderServo, &QSlider::valueChanged, this, [this](int value){
        if (socket.state() != QAbstractSocket::ConnectedState) return;
        if (m_checkRoomba->isChecked()) return;
        QJsonObject comando;
        comando["tipo"] = "servo";
        comando["angulo"] = value;
        socket.sendTextMessage(QJsonDocument(comando).toJson(QJsonDocument::Compact));
    });

    // Controles (Izquierda y Derecha intercambiados)
    connect(m_btnW, &QPushButton::pressed, this, [this](){ enviarComandoManual("avanzar"); });
    connect(m_btnS, &QPushButton::pressed, this, [this](){ enviarComandoManual("retroceder"); });
    connect(m_btnA, &QPushButton::pressed, this, [this](){ enviarComandoManual("derecha"); });
    connect(m_btnD, &QPushButton::pressed, this, [this](){ enviarComandoManual("izquierda"); });

    connect(m_btnW, &QPushButton::released, this, &MainWindow::detenerRobot);
    connect(m_btnS, &QPushButton::released, this, &MainWindow::detenerRobot);
    connect(m_btnA, &QPushButton::released, this, &MainWindow::detenerRobot);
    connect(m_btnD, &QPushButton::released, this, &MainWindow::detenerRobot);

    connect(m_btnSpace, &QPushButton::clicked, this, &MainWindow::detenerRobot);
}

MainWindow::~MainWindow() {}

void MainWindow::on_botonConectar_clicked()
{
    if (socket.state() == QAbstractSocket::ConnectedState) {
        socket.close();
    } else {
        QString ip = m_ip->text();
        socket.open(QUrl("ws://" + ip + ":81"));
    }
}

double MainWindow::aplicarFiltroMediaMovil(double nuevaDistancia)
{
    bufferFiltro.append(nuevaDistancia);
    if (bufferFiltro.size() > 5) bufferFiltro.removeFirst();
    double suma = 0;
    for (double d : bufferFiltro) suma += d;
    return suma / bufferFiltro.size();
}

void MainWindow::onMensajeRecibido(const QString &mensaje)
{
    QJsonDocument doc = QJsonDocument::fromJson(mensaje.toUtf8());
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    QString tipo = obj["tipo"].toString();

    if (tipo == "ultrasonico" && obj["ok"].toBool() == true) {
        double distanciaCruda = obj["dist"].toDouble();
        if (distanciaCruda >= 2.0 && distanciaCruda <= 400.0) {
            double distanciaFiltrada = aplicarFiltroMediaMovil(distanciaCruda);
            m_etiquetaDistancia->setText(QString::number(distanciaFiltrada, 'f', 1) + " cm");
            m_barraProximidad->setValue((int)distanciaFiltrada);

            if (distanciaFiltrada < 20.0) {
                m_barraProximidad->setStyleSheet("QProgressBar::chunk { background-color: red; }");
            } else {
                m_barraProximidad->setStyleSheet("QProgressBar::chunk { background-color: #0078d7; }");
            }

            double t = reloj.elapsed() / 1000.0;
            serie->append(t, distanciaFiltrada);
            if (t > 10) ejeTiempo->setRange(t - 10, t);
            if (serie->count() > 200) serie->remove(0);
        }
    }
    // Lógica DHT
    else if (tipo == "dht_lectura") {
        registrar("RX " + mensaje);
        const QJsonValue t = obj.value("temperatura");
        const QJsonValue h = obj.value("humedad"); // Captura Humedad

        if (!obj.value("dht_ok").toBool() || !t.isDouble()) {
            registrar("Lectura inválida: no se guarda");
            return;
        }

        double tempActual = t.toDouble();
        double humActual = h.toDouble();

        QString icono = "";
        QString estiloColor = "";

        // Colores y Emojis
        if (tempActual >= 26.0) {
            icono = " 🔥";
            estiloColor = "background-color: #ffcccc; color: #000000;";
        } else if (tempActual <= 22.0) {
            icono = " ❄️";
            estiloColor = "background-color: #cce5ff; color: #000000;";
        } else {
            icono = " 🌿";
            estiloColor = "background-color: #d9f2d9; color: #000000;";
        }

        m_temperatura->setText(QString::number(tempActual, 'f', 1) + " °C" + icono);
        m_temperatura->setStyleSheet(estiloColor + " font-size: 28px; font-weight: bold; padding: 5px; border-radius: 8px;");
        this->setStyleSheet("QMainWindow { " + estiloColor + " }");

        // Pasa ambas variables al guardado
        guardarTemperatura(tempActual, humActual);
    }
}

void MainWindow::enviarComandoManual(QString accion)
{
    if (socket.state() != QAbstractSocket::ConnectedState) return;
    if (m_checkRoomba->isChecked()) return;

    QJsonObject comando;
    comando["tipo"] = "mover";
    comando["accion"] = accion;
    comando["ms"] = 0;
    socket.sendTextMessage(QJsonDocument(comando).toJson(QJsonDocument::Compact));
}

void MainWindow::detenerRobot()
{
    enviarComandoManual("parar");
}

// INSERT con columna extra
void MainWindow::guardarTemperatura(double celsius, double humedad)
{
    const double c1 = qRound(celsius * 10) / 10.0;
    const double h1 = qRound(humedad * 10) / 10.0;

    QSqlQuery consulta;
    consulta.prepare("INSERT INTO mediciones (fecha_hora, temperatura, fuente, nota, humedad) "
                     "VALUES (:fecha_hora, :temperatura, :fuente, :nota, :humedad)");
    consulta.bindValue(":fecha_hora", QDateTime::currentDateTime().toString(Qt::ISODate));
    consulta.bindValue(":temperatura", c1);
    consulta.bindValue(":fuente", QString::fromUtf8(FUENTE));
    consulta.bindValue(":nota", "");
    consulta.bindValue(":humedad", h1);

    if (!consulta.exec()) {
        registrar("INSERT falló: " + consulta.lastError().text());
        return;
    }
    registrar(QString("Guardada %1 °C, %2% HR con id %3")
                  .arg(c1, 0, 'f', 1).arg(h1, 0, 'f', 1)
                  .arg(consulta.lastInsertId().toString()));
    cargarHistorial();
}

// SELECT con columna extra
void MainWindow::cargarHistorial()
{
    QSqlQuery consulta("SELECT id, fecha_hora, temperatura, nota, humedad FROM mediciones ORDER BY id DESC LIMIT 50");
    if (!consulta.isActive()) {
        registrar("SELECT falló: " + consulta.lastError().text());
        return;
    }

    m_tabla->setRowCount(0);
    while (consulta.next()) {
        const int fila = m_tabla->rowCount();
        m_tabla->insertRow(fila);
        m_tabla->setItem(fila, 0, new QTableWidgetItem(consulta.value(0).toString()));
        m_tabla->setItem(fila, 1, new QTableWidgetItem(consulta.value(1).toString()));
        m_tabla->setItem(fila, 2, new QTableWidgetItem(QString::number(consulta.value(2).toDouble(), 'f', 1)));
        m_tabla->setItem(fila, 3, new QTableWidgetItem(consulta.value(3).toString()));

        // Agrega la humedad a la quinta columna
        QString humStr = consulta.value(4).isNull() ? "-" : QString::number(consulta.value(4).toDouble(), 'f', 1) + " %";
        m_tabla->setItem(fila, 4, new QTableWidgetItem(humStr));
    }
    m_tabla->resizeColumnsToContents();

    QSqlQuery total("SELECT COUNT(*) FROM mediciones");
    if (total.next()) {
        m_total->setText("Mediciones guardadas: " + total.value(0).toString());
    }
}

void MainWindow::registrar(const QString &texto)
{
    m_registro->append(texto);
}

// ==========================================================
// CAPTURA DE TECLADO (IZQUIERDA Y DERECHA INTERCAMBIADOS)
// ==========================================================
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_W: m_btnW->setDown(true); enviarComandoManual("avanzar"); break;
    case Qt::Key_S: m_btnS->setDown(true); enviarComandoManual("retroceder"); break;
    case Qt::Key_A: m_btnA->setDown(true); enviarComandoManual("derecha"); break;
    case Qt::Key_D: m_btnD->setDown(true); enviarComandoManual("izquierda"); break;
    case Qt::Key_Space: m_btnSpace->setDown(true); detenerRobot(); break;
    default: QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_W: m_btnW->setDown(false); detenerRobot(); break;
    case Qt::Key_S: m_btnS->setDown(false); detenerRobot(); break;
    case Qt::Key_A: m_btnA->setDown(false); detenerRobot(); break;
    case Qt::Key_D: m_btnD->setDown(false); detenerRobot(); break;
    case Qt::Key_Space: m_btnSpace->setDown(false); break;
    default: QMainWindow::keyReleaseEvent(event);
    }
}