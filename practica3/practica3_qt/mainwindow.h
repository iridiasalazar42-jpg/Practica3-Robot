#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QElapsedTimer>
#include <QList>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override; // Agregado para detectar cuando sueltas la tecla

private slots:
    void on_botonConectar_clicked();
    void onMensajeRecibido(const QString &mensaje);
    void on_checkRoomba_toggled(bool checked);
    void on_sliderServo_valueChanged(int value);

    // Slots para presionar (iniciar movimiento)
    void on_pushButtonW_pressed();
    void on_pushButtonA_pressed();
    void on_pushButtonS_pressed();
    void on_pushButtonD_pressed();

    // Slots para soltar (detener movimiento)
    void on_pushButtonW_released();
    void on_pushButtonA_released();
    void on_pushButtonS_released();
    void on_pushButtonD_released();

    // Botón de paro de emergencia
    void on_pushButtonSpace_clicked();

private:
    Ui::MainWindow *ui;
    QWebSocket socket;

    QChart *grafica = new QChart;
    QLineSeries *serie = new QLineSeries;
    QValueAxis *ejeTiempo = new QValueAxis;
    QValueAxis *ejeDistancia = new QValueAxis;
    QElapsedTimer reloj;

    QList<double> bufferFiltro;
    double aplicarFiltroMediaMovil(double nuevaDistancia);

    void enviarComandoManual(QString accion);
    void detenerRobot(); // Función auxiliar para detener
};

#endif // MAINWINDOW_H