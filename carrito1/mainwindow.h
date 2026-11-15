#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>



QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    // Funciones para capturar las teclas WASD
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    // Eventos del WebSocket
    void onConnected();
    void onDisconnected();

    // Función central para enviar los comandos JSON
    void enviarComando(QString accion);


    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    QWebSocket m_webSocket;

    QString url = "ws://10.182.149.221:81";
};
#endif // MAINWINDOW_H
