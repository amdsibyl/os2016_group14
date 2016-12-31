#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QtGui>
#include <QObject>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QImage *Image = new QImage( "/Users/HSUAN/os2016_group14/proj_gui/pic/barber_sleep.png" ) ;
    labelImage = Image;

    ui->barber_1->setPixmap(QPixmap::fromImage(*labelImage));
    ui->barber_2->setPixmap(QPixmap::fromImage(*labelImage));
    ui->barber_3->setPixmap(QPixmap::fromImage(*labelImage));

    //gui = this;
}

void test(){
    //gui->changeBarberMode(1,true);
}

void MainWindow::mouseMoveEvent ( QMouseEvent * event )
{
}


void MainWindow::mouseReleaseEvent ( QMouseEvent * event )
{
  if(event->button() == Qt::LeftButton)
  {
      test();
//      return true;
  }
//  return false;
}

QImage *busyImage = new QImage( "/Users/HSUAN/os2016_group14/proj_gui/pic/barber_busy.png" ) ;
QImage *sleepImage = new QImage( "/Users/HSUAN/os2016_group14/proj_gui/pic/barber_sleep.png" ) ;

void MainWindow::changeBarberMode(int i,bool isBusy){
#if 0
    app.processEvents();
#endif
    if(isBusy){
        //cout<<"hi\n";
        *labelImage = *busyImage;
    }
    else{
        *labelImage = *sleepImage;
    }
    if(i==1)
        ui->barber_1->setPixmap(QPixmap::fromImage(*labelImage));
    else if(i==2)
        ui->barber_2->setPixmap(QPixmap::fromImage(*labelImage));
    else if(i==3)
        ui->barber_3->setPixmap(QPixmap::fromImage(*labelImage));
    //a->processEvents();
}


MainWindow::~MainWindow()
{
    delete ui;
}
