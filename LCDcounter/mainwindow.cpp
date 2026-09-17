#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Senal -> slot (Qt 6): al pulsar el boton (con retencion ) y en cada tic del timer .
    connect (ui->btnPlay, &QPushButton::toggled, this, & MainWindow::onPlayToggled);
    connect (&timer, &QTimer::timeout, this, & MainWindow::onTick);
    ui-> lcd-> display(cuenta); // arranca en 0
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onPlayToggled(bool activo)
{
    if (activo) {
        ui->btnPlay->setText("Pausar");
        timer.start(100); // un tic por segundo (1000 ms)
    } else {
        ui->btnPlay->setText("Iniciar");
        timer.stop();
    }
}
// En cada tic : subimos el contador y lo mostramos en el LCD.
void MainWindow::onTick ()
{
    ++cuenta;
    ui->lcd->display(cuenta) ;
}