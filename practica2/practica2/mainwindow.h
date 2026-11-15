#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QElapsedTimer>
#include <QList>
#include <QKeyEvent> // <--- NUEVO: Librería para detectar teclas

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // <--- NUEVO: Evento para capturar el teclado
protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_botonConectar_clicked();
    void onMensajeRecibido(const QString &mensaje);
    void on_checkRoomba_toggled(bool checked);

    // <--- NUEVO: Slots de los botones de movimiento
    void on_pushButtonW_clicked();
    void on_pushButtonA_clicked();
    void on_pushButtonS_clicked();
    void on_pushButtonD_clicked();
    void on_pushButtonSpace_clicked();
    void on_sliderServo_valueChanged(int value);

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

    // <--- NUEVO: Función auxiliar para no repetir código
    void enviarComandoManual(QString accion);
};

#endif // MAINWINDOW_H