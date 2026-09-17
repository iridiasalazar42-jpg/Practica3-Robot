/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *btnAbrir;
    QPushButton *btnPlay;
    QPushButton *btnStop;
    QCheckBox *chkGris;
    QCheckBox *chkEspejo;
    QCheckBox *chkLoop;
    QLabel *lblVideo;
    QLabel *lbEstado;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        btnAbrir = new QPushButton(centralwidget);
        btnAbrir->setObjectName("btnAbrir");
        btnAbrir->setGeometry(QRect(30, 50, 94, 26));
        btnPlay = new QPushButton(centralwidget);
        btnPlay->setObjectName("btnPlay");
        btnPlay->setGeometry(QRect(30, 100, 94, 26));
        btnPlay->setCheckable(true);
        btnStop = new QPushButton(centralwidget);
        btnStop->setObjectName("btnStop");
        btnStop->setGeometry(QRect(30, 150, 94, 26));
        chkGris = new QCheckBox(centralwidget);
        chkGris->setObjectName("chkGris");
        chkGris->setGeometry(QRect(30, 230, 91, 23));
        chkEspejo = new QCheckBox(centralwidget);
        chkEspejo->setObjectName("chkEspejo");
        chkEspejo->setGeometry(QRect(30, 260, 91, 23));
        chkLoop = new QCheckBox(centralwidget);
        chkLoop->setObjectName("chkLoop");
        chkLoop->setGeometry(QRect(30, 200, 91, 23));
        lblVideo = new QLabel(centralwidget);
        lblVideo->setObjectName("lblVideo");
        lblVideo->setGeometry(QRect(250, 40, 451, 221));
        lbEstado = new QLabel(centralwidget);
        lbEstado->setObjectName("lbEstado");
        lbEstado->setGeometry(QRect(290, 290, 311, 18));
        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btnAbrir->setText(QCoreApplication::translate("MainWindow", "btnAbrir", nullptr));
        btnPlay->setText(QCoreApplication::translate("MainWindow", "btnPlay", nullptr));
        btnStop->setText(QCoreApplication::translate("MainWindow", "btnStop", nullptr));
        chkGris->setText(QCoreApplication::translate("MainWindow", "chkGris", nullptr));
        chkEspejo->setText(QCoreApplication::translate("MainWindow", "chkEspejo", nullptr));
        chkLoop->setText(QCoreApplication::translate("MainWindow", "chkLoop", nullptr));
        lblVideo->setText(QCoreApplication::translate("MainWindow", "lblVideo", nullptr));
        lbEstado->setText(QCoreApplication::translate("MainWindow", "lbEstado", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
