#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMap>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QByteArray>
#include <QTimer>

struct TagData {
    QString name;
    int points;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleConnection();
    void readSerialData();
    void saveTagData();
    void updateTable();
    void refreshPorts();
    void onPLayToggle(bool activo);
    void onTick();



private:
    void setupUI();
    void applyStyleSheet(); // Nueva función para embellecer la UI

    // UI Elements
    QComboBox *portComboBox;
    QPushButton *connectBtn;
    QPushButton *refreshBtn;
    QLabel *statusLabel;
    QLabel *currentUidLabel;
    QLineEdit *nameInput;
    QSpinBox *pointsInput;
    QPushButton *saveBtn;
    QTableWidget *table;
    QTimer timer;
    int cuenta=0;

    // Data
    QString currentUID;
    QMap<QString, TagData> database;
    QByteArray serialBuffer;

    // Serial
    QSerialPort *serial;
};
#endif // MAINWINDOW_H