#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QtGui>
#include <QObject>
#include <iostream>

QImage *sleepImage = new QImage( "C:/Users/Amanda/Desktop/proj_gui win/pic/barber_sleep.png" ) ;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    std::cout<<"guigui2222222_"<<this<<std::endl;
    ui->setupUi(this);

    labelImage = sleepImage;

    ui->barber_1->setPixmap(QPixmap::fromImage(*labelImage));
    ui->barber_2->setPixmap(QPixmap::fromImage(*labelImage));
    ui->barber_3->setPixmap(QPixmap::fromImage(*labelImage));

    //gui = this;
}

void test(){
    //gui->changeBarberMode(1,true);
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

QImage *busyImage = new QImage( "C:/Users/Amanda/Desktop/proj_gui win/pic/barber_busy.png" ) ;


void MainWindow::changeBarberMode(int i,bool isBusy){
#if 0
    app.processEvents();
#endif


    if(isBusy){
        std::cout<<"isbusy:"<<i<<std::endl;

        *labelImage = *busyImage;
    }
    else{
        *labelImage = *sleepImage;
         std::cout<<"isnotbusy:"<<i<<std::endl;
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
