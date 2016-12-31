#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QImage>
#include <QToolButton>
#include <QPixmap>
#include <QFileDialog>
#include <QSize>
#include <iostream>
#include <QMouseEvent>


void runProgress(){
    for(long i=0;i<10000000;i++);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

void MainWindow::mousePressEvent(QMouseEvent *event){

      if(event->button() == Qt::LeftButton)
      {
          QImage my;
          my.load("C:/Users/Amanda/Desktop/mousePress/barber_sleep.png");
          ui->toolButton->setIconSize(QSize(my.width(),my.height()));
          ui->toolButton->setIcon(QPixmap::fromImage(my));
      }



}


MainWindow::~MainWindow()
{
    delete ui;
}
