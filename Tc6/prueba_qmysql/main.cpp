// TC06-ID: 223823764-IGSE
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QTextStream salida(stdout);

    salida << "TC06-ID: 223823764-IGSE" << Qt::endl;
    salida << "Drivers: " << QSqlDatabase::drivers().join(", ") << Qt::endl;

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setUserName("curso");
    db.setPassword("curso1234");

    if (db.open()) {
        salida << "Conectado al servidor MySQL." << Qt::endl;
        QSqlQuery consulta("SELECT VERSION()");
        if (consulta.next()) {
            salida << "Servidor: " << consulta.value(0).toString() << Qt::endl;
        }
    } else {
        salida << "Error: " << db.lastError().text() << Qt::endl;
    }
    return 0;
}