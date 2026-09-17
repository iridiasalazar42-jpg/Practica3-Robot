// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
// Diseno de Interfaces (I7262) 2026B

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTick);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
void MainWindow::on_btnAbrir_clicked()
{
    QString ruta = QFileDialog::getOpenFileName(this, "Abrir Video", "", "Video MP4 (*.mp4)");

    if (ruta.isEmpty())
    {
        return;
    }

    if(!cap.open(ruta.toStdString()))
    {
        return;
    }

    setWindowTitle("ReproductorMP4 - Iridia Guadalupe Salazar Esmerio (223823764) - " + QFileInfo(ruta).fileName());

    cv::Mat frame;

    if (cap.read(frame)) {

        cv::Mat frameRGB;
        cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);

        QImage img((const unsigned char*)(frameRGB.data),
                   frameRGB.cols, frameRGB.rows,
                   frameRGB.step, QImage::Format_RGB888);

        ui->lblVideo->setPixmap(QPixmap::fromImage(img.copy()).scaled(ui->lblVideo->size(), Qt::KeepAspectRatio));
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    int totalCuadros = cap.get(cv::CAP_PROP_FRAME_COUNT);

    ui->lbEstado->setText(QString("Cargado: %1 cuadros a %2 fps").arg(totalCuadros).arg(fps));
    ui->btnPlay->setEnabled(true);
    ui->btnPlay->setChecked(false);
}

// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
void MainWindow::on_btnPlay_toggled(bool checked)
{
    if (checked) {
        ui->btnPlay->setText("Pausa");
        double fps = cap.get(cv::CAP_PROP_FPS);
        timer->start(1000 / fps); // Arranca con periodo 1000/fps ms
    } else {
        ui->btnPlay->setText("Play");
        timer->stop();
    }
}

// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
bool MainWindow::leerYMostrarCuadro()
{
    cv::Mat frame;
    if (!cap.read(frame)) {
        return false; // El video terminó
    }

    cv::Mat frameRGB;

    // --- PASO 7: Lógica de Escala de Grises ---
    if (ui->chkGris->isChecked()) {
        cv::Mat frameGris;
        // Convertimos de BGR a Grises
        cv::cvtColor(frame, frameGris, cv::COLOR_BGR2GRAY);
        // Volvemos a convertir a RGB para que sea compatible con Format_RGB888 de Qt
        cv::cvtColor(frameGris, frameRGB, cv::COLOR_GRAY2RGB);
    } else {
        // Conversión normal si la casilla no está marcada
        cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);
    }

    // --- PASO 8: Lógica de Espejo ---
    if (ui->chkEspejo->isChecked()) {
        cv::flip(frameRGB, frameRGB, 1);
    }

    // Crear la imagen para Qt
    QImage img((const unsigned char*)(frameRGB.data),
               frameRGB.cols, frameRGB.rows,
               frameRGB.step, QImage::Format_RGB888);

    ui->lblVideo->setPixmap(QPixmap::fromImage(img.copy()).scaled(ui->lblVideo->size(), Qt::KeepAspectRatio));

    return true;
}

// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
void MainWindow::onTick()
{
    if (leerYMostrarCuadro()) {
        // El video sigue avanzando
        int cuadroActual = cap.get(cv::CAP_PROP_POS_FRAMES);
        int totalCuadros = cap.get(cv::CAP_PROP_FRAME_COUNT);
        double fps = cap.get(cv::CAP_PROP_FPS);

        ui->lbEstado->setText(QString("Cuadro %1 / %2  (%3 fps)")
                                   .arg(cuadroActual).arg(totalCuadros).arg(fps));
    } else {
        // --- Lógica de Fin de Video (PASO 9) y Recarga (RETO C - PASO 10) ---
        if (ui->chkLoop->isChecked()) {
            // Si "Repetir" está marcado, rebobina y el video continúa reproduciéndose solo
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        } else {
            // Reto C: Si termina sin Repetir, simulamos que el usuario presionó Detener
            timer->stop();
            on_btnStop_clicked();
            ui->lbEstado->setText("Fin del video");
        }
    }
}

// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
void MainWindow::on_btnStop_clicked()
{
    // 1. Soltar el botón Play (esto automáticamente detiene el QTimer gracias a on_btnPlay_toggled)
    ui->btnPlay->setChecked(false);

    // 2. Rebobinar el video al cuadro 0
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    // 3. Leer y mostrar ese primer cuadro
    leerYMostrarCuadro();

    // 4. Restaurar la etiqueta de estado a "Cargado"
    double fps = cap.get(cv::CAP_PROP_FPS);
    int totalCuadros = cap.get(cv::CAP_PROP_FRAME_COUNT);
    ui->lbEstado->setText(QString("Cargado: %1 cuadros a %2 fps").arg(totalCuadros).arg(fps));
}
void MainWindow::on_chkLoop_clicked()
{

}

