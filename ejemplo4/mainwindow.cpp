#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

void MainWindow::alconectar()
{

}
void MainWindow::alrecibirmensaje(const QString &mensaje)
{

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&socket, &QWebSocket::connected, this, &MainWindow::alconectar);
    connect(&socket, &QWebSocket::textMessageReceived, this, &MainWindow::alrecibirmensaje);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString ip = ui->lineEdit->text().trimmed();
    if(ip.isEmpty())return;

    if(socket.state() != QAbstractSocket::ConnectedState)
    {
       socket.open(QUrl(ip));
    }
    else
    {
        QJsonObject peticion;
        peticion["tipo"] = "leer_red";
        socket.sendTextMessage(QJsonDocument(peticion).toJson(QJsonDocument::Compact));
    }


}