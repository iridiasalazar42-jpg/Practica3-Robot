#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWebSockets>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

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

private slots:
    void on_pushButton_clicked(bool checked);
    void enviarJson(const QJsonObject &objeto);
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked(bool checked);
    void on_pushButton_4_clicked(bool checked);
    void on_pushButton_5_clicked(bool checked);

    // Slots añadidos para manejar los eventos del WebSocket
    void onConnected();
    void onTextMessageReceived(QString message);

private:
    Ui::MainWindow *ui;
    QWebSocket socket;

    QString verdeEncencido =
        "QPushButton{background:#2ecc71;"
        "border-radius:70px; border:2px solid #27ae60;"
        "min-width:70px; min-height70px;}";

    QString grisApagado =
        "QPushButton{background:#95a5a6;"
        "border-radius:70px; border:2px solid #7f8c8d;"
        "min-width:70px; min-height70px;}";

};
#endif // MAINWINDOW_H