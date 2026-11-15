#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
// --- Agrega estos includes debajo del QMainWindow ---
#include <QWebSocket>
#include <QTimer>
#include <QJsonObject>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_botonConectar_clicked();
    void on_deslizadorAngulo_valueChanged(int value);
    void on_botonConsultar_clicked();
    void on_botonBarrido_clicked();

private:
    Ui::MainWindow *ui;

    // --- Agrega esto debajo de ui; ---
    QWebSocket socket;          // cliente WebSocket (objeto, no puntero)
    QTimer temporizador;        // limite de envio: 1 mensaje cada 50 ms
    int anguloPendiente = -1;   // ultimo angulo pedido que falta enviar

    // Grafica del potenciometro.
    QChart *grafica = new QChart;
    QLineSeries *serie = new QLineSeries;
    QValueAxis *ejeTiempo = new QValueAxis;
    QValueAxis *ejeVoltaje = new QValueAxis;
    QElapsedTimer reloj;        // segundos desde que se conecto

    void enviarJson(const QJsonObject &objeto);
    void registrar(const QString &linea);
    void agregarLectura(double voltaje);
};
#endif // MAINWINDOW_H