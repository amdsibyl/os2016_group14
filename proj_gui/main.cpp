#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
/*
    myMainWindow *window = new myMainWindow();

        window->setWindowTitle(QString::fromUtf8("QT - Capture Mouse Left Click"));
        window->resize(300, 250);

    window->show();
    */
    MainWindow w;

    w.show();

    return a.exec();
}
