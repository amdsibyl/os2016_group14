#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QImage>
#include <QToolButton>
#include <QPixmap>
#include <QFileDialog>
#include <QSize>
#include <iostream>

void runProgress(){
    for(long i=0;i<10000000;i++);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*
    QImage my;
    my.load("/Users/HSUAN/untitled/barber_sleep.png");
    ui->toolButton->setIconSize(QSize(my.width(),my.height()));
    ui->toolButton->setIcon(QPixmap::fromImage(my));
    */
}

void MainWindow::mousePressEvent(){
    std::cout<<"hi\n";
    /*
    QString fName;
    QImage myImg;
    fName = QFileDialog::getOpenFileName(this, "/Users/HSUAN/untitled/barber_sleep.png");
    myImg.load(fName);
    ui->toolButton->setIconSize(QSize(myImg.width(),myImg.height()));
    ui->toolButton->setIcon(QPixmap::fromImage(myImg));
    */
    QImage my;
    my.load("/Users/HSUAN/untitled/barber_sleep.png");
    ui->toolButton->setIconSize(QSize(my.width(),my.height()));
    ui->toolButton->setIcon(QPixmap::fromImage(my));

}

MainWindow::~MainWindow()
{
    delete ui;
}
