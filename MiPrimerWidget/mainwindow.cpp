#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFont>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    serial = new QSerialPort(this);
    setupUI();
    applyStyleSheet(); // Aplicamos los estilos modernos
    refreshPorts();

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
}

MainWindow::~MainWindow()
{
    if (serial->isOpen()) {
        serial->close();
    }
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ==========================================
    // SECCIÓN 1: Conexión Serial
    // ==========================================
    QGroupBox *connGroup = new QGroupBox("Conexión con ESP32-S3", this);
    QHBoxLayout *connLayout = new QHBoxLayout(connGroup);
    connLayout->setSpacing(10);

    portComboBox = new QComboBox(this);
    portComboBox->setMinimumWidth(200);

    refreshBtn = new QPushButton("Refrescar", this);
    refreshBtn->setObjectName("refreshBtn");

    connectBtn = new QPushButton("Conectar", this);
    connectBtn->setObjectName("actionBtn"); // Para el estilo CSS

    statusLabel = new QLabel("Estado: Desconectado", this);
    statusLabel->setObjectName("statusLabel");

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);

    connLayout->addWidget(new QLabel("Puerto USB:", this));
    connLayout->addWidget(portComboBox);
    connLayout->addWidget(refreshBtn);
    connLayout->addWidget(connectBtn);
    connLayout->addStretch();
    connLayout->addWidget(statusLabel);

    // ==========================================
    // SECCIÓN 2: Lectura y Registro de Tarjeta
    // ==========================================
    QGroupBox *tagGroup = new QGroupBox("Información de la Tarjeta RFID", this);
    QVBoxLayout *tagLayout = new QVBoxLayout(tagGroup);

    // Fila superior (UID)
    QHBoxLayout *uidLayout = new QHBoxLayout();
    currentUidLabel = new QLabel("Esperando tarjeta...", this);
    currentUidLabel->setObjectName("uidLabel");
    currentUidLabel->setAlignment(Qt::AlignCenter);
    uidLayout->addWidget(currentUidLabel);

    // Fila inferior (Formulario)
    QHBoxLayout *formLayout = new QHBoxLayout();
    nameInput = new QLineEdit(this);
    nameInput->setPlaceholderText("Nombre del Usuario (Ej. Juan Pérez)");

    pointsInput = new QSpinBox(this);
    pointsInput->setRange(0, 99999);
    pointsInput->setPrefix("Puntos: ");
    pointsInput->setMinimumWidth(120);

    saveBtn = new QPushButton("Guardar / Actualizar Datos", this);
    saveBtn->setObjectName("saveBtn");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveTagData);

    formLayout->addWidget(new QLabel("Nombre:", this));
    formLayout->addWidget(nameInput);
    formLayout->addWidget(pointsInput);
    formLayout->addWidget(saveBtn);

    tagLayout->addLayout(uidLayout);
    tagLayout->addSpacing(10);
    tagLayout->addLayout(formLayout);

    // ==========================================
    // SECCIÓN 3: Tabla de Base de Datos
    // ==========================================
    QGroupBox *dbGroup = new QGroupBox("Base de Datos de Usuarios", this);
    QVBoxLayout *dbLayout = new QVBoxLayout(dbGroup);

    table = new QTableWidget(0, 3, this);
    table->setHorizontalHeaderLabels({"UID de Tarjeta", "Nombre de Usuario", "Puntos Acumulados"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    dbLayout->addWidget(table);

    // Agregar todo al layout principal
    mainLayout->addWidget(connGroup);
    mainLayout->addWidget(tagGroup);
    mainLayout->addWidget(dbGroup);

    setCentralWidget(central);
    setMinimumSize(750, 550);
    setWindowTitle("Sistema de Control RFID - ESP32");
}

void MainWindow::applyStyleSheet()
{
    // Hoja de estilos (CSS adaptado a Qt) para un diseño moderno y limpio
    QString style = R"(
        QMainWindow {
            background-color: #f4f6f9;
        }
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
            border: 1px solid #d1d5db;
            border-radius: 8px;
            margin-top: 15px;
            background-color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 15px;
            color: #1f2937;
            padding: 0 5px;
        }
        QLabel {
            color: #374151;
            font-size: 13px;
        }
        QLabel#statusLabel {
            font-weight: bold;
            color: #6b7280;
        }
        QLabel#uidLabel {
            font-size: 24px;
            font-weight: bold;
            color: #2563eb;
            background-color: #eff6ff;
            border: 2px dashed #93c5fd;
            border-radius: 8px;
            padding: 15px;
        }
        QLineEdit, QSpinBox, QComboBox {
            border: 1px solid #d1d5db;
            border-radius: 5px;
            padding: 6px 10px;
            background-color: #f9fafb;
            color: #1f2937;
            font-size: 13px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1px solid #3b82f6;
            background-color: #ffffff;
        }
        QPushButton {
            background-color: #e5e7eb;
            color: #374151;
            border: none;
            border-radius: 5px;
            padding: 8px 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #d1d5db;
        }
        QPushButton#actionBtn {
            background-color: #10b981;
            color: white;
        }
        QPushButton#actionBtn:hover {
            background-color: #059669;
        }
        QPushButton#saveBtn {
            background-color: #3b82f6;
            color: white;
        }
        QPushButton#saveBtn:hover {
            background-color: #2563eb;
        }
        QTableWidget {
            border: 1px solid #e5e7eb;
            background-color: #ffffff;
            alternate-background-color: #f9fafb;
            selection-background-color: #bfdbfe;
            selection-color: #1e3a8a;
            gridline-color: #f3f4f6;
        }
        QHeaderView::section {
            background-color: #1f2937;
            color: #ffffff;
            font-weight: bold;
            padding: 8px;
            border: none;
            border-right: 1px solid #374151;
        }
    )";
    this->setStyleSheet(style);
}

void MainWindow::refreshPorts()
{
    portComboBox->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        portComboBox->addItem(info.portName(), info.systemLocation());
    }
}

void MainWindow::toggleConnection()
{
    if (serial->isOpen()) {
        serial->close();
        connectBtn->setText("Conectar");
        connectBtn->setStyleSheet(""); // Restablecer estilo
        statusLabel->setText("Estado: Desconectado");
        portComboBox->setEnabled(true);
        refreshBtn->setEnabled(true);
    } else {
        if (portComboBox->currentIndex() == -1) {
            QMessageBox::warning(this, "Error", "No hay puertos seleccionados.");
            return;
        }

        serial->setPortName(portComboBox->currentData().toString());
        serial->setBaudRate(QSerialPort::Baud115200);
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);

        if (serial->open(QIODevice::ReadWrite)) {
            connectBtn->setText("Desconectar");
            connectBtn->setStyleSheet("background-color: #ef4444; color: white;"); // Rojo al desconectar
            statusLabel->setText("Estado: Conectado");
            portComboBox->setEnabled(false);
            refreshBtn->setEnabled(false);
        } else {
            QMessageBox::critical(this, "Error", "No se pudo abrir el puerto.\nRevisa los permisos (dialout).");
        }
    }
}

void MainWindow::readSerialData()
{
    serialBuffer.append(serial->readAll());

    while (serialBuffer.contains('\n')) {
        int newlineIndex = serialBuffer.indexOf('\n');
        QByteArray line = serialBuffer.left(newlineIndex).trimmed();
        serialBuffer.remove(0, newlineIndex + 1);

        QString dataStr = QString::fromStdString(line.toStdString());

        if (!dataStr.isEmpty() && !dataStr.contains(" ")) {
            currentUID = dataStr;
            currentUidLabel->setText("UID Detectado: " + currentUID);

            if (database.contains(currentUID)) {
                nameInput->setText(database[currentUID].name);
                pointsInput->setValue(database[currentUID].points);
            } else {
                nameInput->clear();
                pointsInput->setValue(0);
            }
        }
    }
}

void MainWindow::saveTagData()
{
    if (currentUID.isEmpty()) {
        QMessageBox::warning(this, "Atención", "Por favor, escanea una tarjeta primero antes de guardar.");
        return;
    }

    if (nameInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Atención", "El nombre no puede estar vacío.");
        return;
    }

    TagData data;
    data.name = nameInput->text().trimmed();
    data.points = pointsInput->value();

    database[currentUID] = data;
    updateTable();

    QMessageBox::information(this, "Éxito", "Datos de la tarjeta guardados correctamente.");
}

void MainWindow::updateTable()
{
    table->setRowCount(0);
    int row = 0;
    for (auto it = database.begin(); it != database.end(); ++it) {
        table->insertRow(row);

        QTableWidgetItem *uidItem = new QTableWidgetItem(it.key());
        uidItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *nameItem = new QTableWidgetItem(it.value().name);

        QTableWidgetItem *pointsItem = new QTableWidgetItem(QString::number(it.value().points));
        pointsItem->setTextAlignment(Qt::AlignCenter);

        table->setItem(row, 0, uidItem);
        table->setItem(row, 1, nameItem);
        table->setItem(row, 2, pointsItem);
        row++;
    }
}