#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>

class QLineEdit;
class QPushButton;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void alConectar();
    void alConectarSocket();
    void alDesconectarSocket();
    void alRecibirMensaje(const QString &mensaje);

private:
    void mostrarEstado(bool encendido);

    // Punteros para los elementos de la interfaz creados por código
    QLineEdit   *campoIp;
    QPushButton *botonConectar;
    QLabel      *indicador;
    QLabel      *etiquetaAyuda;
    QWebSocket  socket;
};

#endif // MAINWINDOW_H