#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QChartView>
#include <QElapsedTimer>
#include <QList>
#include <QKeyEvent>

// Includes para la UI dinámica (Sin .ui)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QSlider>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimer>
#include <QTableWidget>
#include <QSpinBox>

// Includes para Base de Datos
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void on_botonConectar_clicked();
    void onMensajeRecibido(const QString &mensaje);
    void cargarHistorial(); // Para actualizar la tabla

private:
    // --- CONSTANTES P4 ---
    const QString ALUMNO = "Iridia Salazar 223823764 temperatura_3764.db";
    static constexpr const char *FUENTE = "dht22"; // Cambia a "simulada" si usas el #define SIMULAR_DHT true

    // --- VARIABLES RED Y LÓGICA ---
    QWebSocket socket;
    QElapsedTimer reloj;
    QList<double> bufferFiltro;
    double aplicarFiltroMediaMovil(double nuevaDistancia);

    void enviarComandoManual(QString accion);
    void detenerRobot();
    void registrar(const QString &texto);
    void guardarTemperatura(double celsius, double humedad);

    // --- WIDGETS PESTAÑA ROBOT (P3 convertida a código) ---
    QLineEdit *m_ip;
    QPushButton *m_btnConectar;
    QCheckBox *m_checkRoomba;
    QLabel *m_etiquetaDistancia;
    QProgressBar *m_barraProximidad;
    QChart *grafica;
    QLineSeries *serie;
    QValueAxis *ejeTiempo;
    QValueAxis *ejeDistancia;
    QChartView *m_graficaDistancia;
    QSlider *m_sliderServo;
    QPushButton *m_btnW, *m_btnA, *m_btnS, *m_btnD, *m_btnSpace;
    QTextEdit *m_registro; // Agregado para ver los JSON

    // --- WIDGETS PESTAÑA TEMPERATURA (P4) ---
    QLabel *m_temperatura;
    QCheckBox *m_autoTemp;
    QTimer *m_timerTemp;
    QTableWidget *m_tabla;
    QLabel *m_total;
    QSpinBox *m_id;
    QLineEdit *m_nota;
    QLabel *m_statsExtra;
};

#endif // MAINWINDOW_H