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
    void mouseReleaseEvent ( QMouseEvent * event )
    {
      if(event->button() == Qt::LeftButton)
      {
        msgBox = new QMessageBox();
        msgBox->setWindowTitle("Hello");
        msgBox->setText("You Clicked Left Mouse Button");
        msgBox->show();
      }
    };

};


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    myMainWindow *window = new myMainWindow();

        window->setWindowTitle(QString::fromUtf8("QT - Capture Mouse Left Click"));
        window->resize(300, 250);

    window->show();


    MainWindow w;
    w.show();

    return a.exec();
}
