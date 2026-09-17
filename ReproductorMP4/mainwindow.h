// TC3 Reproductor MP4 - Iridia Guadalupe Salazar Esmerio - Codigo 223823764
// Diseno de Interfaces (I7262) 2026B

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <QTimer>

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
    void on_btnAbrir_clicked();
    void onTick();
    void on_btnPlay_toggled(bool checked);

    void on_chkLoop_clicked();

    void on_btnStop_clicked();

private:
    Ui::MainWindow *ui;
    cv::VideoCapture cap;
    QTimer *timer;

    bool leerYMostrarCuadro();
};
#endif // MAINWINDOW_H
