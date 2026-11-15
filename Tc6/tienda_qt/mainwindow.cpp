// TC06-ID: 223823764-IGSE

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QMessageBox>
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_botonGuardar_clicked()
{
    QSqlQuery consulta;
    consulta.prepare("INSERT INTO productos (nombre, precio, existencia, categoria) "
                     "VALUES (:nombre, :precio, :existencia, :categoria)");

    // Tomamos los datos de la interfaz visual
    consulta.bindValue(":nombre", ui->entraNombre->text());
    consulta.bindValue(":precio", ui->spinPrecio->value());
    consulta.bindValue(":existencia", ui->spinExistencia->value());
    consulta.bindValue(":categoria", ui->entraCategoria->text());

    if (consulta.exec()) {
        QMessageBox::information(this, "Éxito", "Producto guardado en la base de datos.");
        // Opcional: Llamar a la función del botón Cargar para que la tabla se refresque automáticamente
        on_Cargar_clicked();
    } else {
        QMessageBox::critical(this, "Error", "No se pudo guardar: " + consulta.lastError().text());
    }
}


void MainWindow::on_Cargar_clicked()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("tienda");
    db.setUserName("curso");
    db.setPassword("curso1234");

    if (db.open()) {
        QSqlQueryModel *modelo = new QSqlQueryModel(this);
        modelo->setQuery("SELECT * FROM productos");
        ui->tableView->setModel(modelo);
    } else {
        QMessageBox::critical(this, "Error", "No se pudo conectar: " + db.lastError().text());
    }
}

