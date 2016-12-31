#include "mainwindow.h"
#include <QApplication>

#include <QMainWindow>
#include <QMouseEvent>
#include <QMessageBox>


class myMainWindow: public QMainWindow
{
  QMessageBox* msgBox;
  public:
    myMainWindow()
    {};
    ~ myMainWindow(){};

    //Click event = MousePress->MouseRelease


};


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
