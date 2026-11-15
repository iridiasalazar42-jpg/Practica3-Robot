#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>

class QLineEdit;
class QPushButton;
class QLabel;

// Empareja con el sketch 01_boton_retencion_led por WebSocket (ws://<IP>:81):
// recibe "LED:ON"/"LED:OFF" y refleja el LED de la ESP32 en tiempo real.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void alConectar();
    void alConectarSocket();
    void alDesconectarSocket();
    void alRecibirMensaje(const QString &mensaje);

private:
    void mostrarEstado(bool encendido);

    QLineEdit  *campoIp;
    QPushButton *botonConectar;
    QLabel     *indicador;
    QLabel     *etiquetaAyuda;
    QWebSocket  socket;
};

#endif // MAINWINDOW_H
