#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    //void mouseMoveEvent ( QMouseEvent * event );
    void mouseReleaseEvent( QMouseEvent * event );
    void changeBarberMode(int i,bool isBusy);
    Ui::MainWindow* getUI(){return ui;}

    ~MainWindow();

/*private:*/
    Ui::MainWindow *ui;
    QImage *labelImage;
};

#endif // MAINWINDOW_H
