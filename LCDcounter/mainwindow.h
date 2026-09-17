#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
# include <QTimer>
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


private slots :
    void onPlayToggled (bool activo);
    void onTick();

private:
    Ui::MainWindow *ui;
    QTimer timer; // el " reloj " que marca el ritmo del conteo
    int cuenta=0; // el numero que mostramos
};
#endif // MAINWINDOW_H
