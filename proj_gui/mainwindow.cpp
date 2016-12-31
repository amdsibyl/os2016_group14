#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QtGui>

#include <fcntl.h>
#include <iostream>
#include <dispatch/dispatch.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <random>
#include <iomanip>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

#define MEAN 3
#define TIME_RANGE 10
#define NUM_CUSTOMER 3

using namespace std;

MainWindow *gui;

/*
 * The barber shop has m barbers with m barber chairs,
 * and n chairs (m < n) for waiting customers, if any, to sit in.
 * If there are no customers present, a barber sits down in a barber chair and falls asleep.
 * When a customer arrives, he has to wake up a sleeping barber.
 * If additional customers arrive while all barbers are cutting customers' hair,
 * they either sit down (if there are empty chairs) or leave the shop (if all chairs are full).
 * The thread synchronization problem is to program the barbers and the customers
 * without getting into race conditions.
 */

/*Shared data*/
dispatch_semaphore_t barbers = dispatch_semaphore_create(NUM_BARBERS);      /*Number of barbers waiting for customers*/
dispatch_semaphore_t customers = dispatch_semaphore_create(0);    /*Number of customers waiting for service*/
dispatch_semaphore_t Mutex = dispatch_semaphore_create(1);        /*Mutex used for mutual exclusion*/
dispatch_semaphore_t ioMutex = dispatch_semaphore_create(1);      /*Mutex used for input and output*/
dispatch_semaphore_t cusMutex = dispatch_semaphore_create(1);     /*Mutex used for change totalServedCustomers*/
dispatch_semaphore_t barMutex = dispatch_semaphore_create(1);     /*Mutex used for one check per time*/

int cutting[NUM_BARBERS];
bool isBusy[NUM_BARBERS];
int seat[NUM_CHAIRS];
bool isSit[NUM_CHAIRS];
int comeCus[NUM_CUSTOMER];

void print(){
    cout<<"\n@@0 busy: "<<isBusy[0]<<endl;
    cout<<"\n@@1 busy: "<<isBusy[1]<<endl;
    cout<<"\n@@2 busy: "<<isBusy[2]<<endl;
    cout<<"----------------------------------------------\n";
    cout<<"|                                            |\n";
    cout<<"| Barber                                     |\n";

    cout<<"|  ";
    for(int i=0;i<NUM_BARBERS;i++)
        cout<<" "<<setfill('0')<<setw(2)<<i+1<<"   ";
    for(int i=0;i<7-NUM_BARBERS;i++)
        cout<<"      ";
    cout<<"|\n";	//"01"

    cout<<"|  ";
    for(int i=0;i<NUM_BARBERS;i++)
        cout<<"----  ";
    for(int i=0;i<7-NUM_BARBERS;i++)
        cout<<"      ";
    cout<<"|\n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|  ";
    for(int i=0;i<NUM_BARBERS;i++){
        if(isBusy[i]&&cutting[i]!=-1){
            cout<<"|"<<setfill('0')<<setw(2)<<cutting[i]<<"|  ";
        }
        else
            cout<<"|--|  ";
    }
    for(int i=0;i<7-NUM_BARBERS;i++)
        cout<<"      ";
    cout<<"|\n";	//"| 1|  "

    cout<<"|  ";
    for(int i=0;i<NUM_BARBERS;i++)
        cout<<"----  ";
    for(int i=0;i<7-NUM_BARBERS;i++)
        cout<<"      ";
    cout<<"|\n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|  Chair                                    / \n";

    cout<<"|  ";
    for(int i=0;i<NUM_CHAIRS;i++)
        cout<<"----  ";
    for(int i=0;i<6-NUM_CHAIRS;i++)
        cout<<"      ";
    cout<<"    /  \n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|  ";
    for(int i=0;i<NUM_CHAIRS;i++){
        if(isSit[i] && seat[i]!=-1){
            cout<<"|"<<setfill('0')<<setw(2)<<seat[i]<<"|  ";
        }
        else
            cout<<"|--|  ";
    }
    for(int i=0;i<6-NUM_CHAIRS;i++)
        cout<<"      ";
    cout<<"   / ";
    // coming customers
    for(int i=0;i<NUM_CUSTOMER;i++){
        if(comeCus[i]==true)
            cout<<i+1<<" ";
    }
    cout<<"\n";	//cout<<"|                                         /   \n";

    cout<<"|  ";
    for(int i=0;i<NUM_CHAIRS;i++)
        cout<<"----  ";
    for(int i=0;i<6-NUM_CHAIRS;i++)
        cout<<"      ";
    cout<<"  /    \n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|                                            |\n";
    cout<<"----------------------------------------------\n";
}

struct customerData
{
    int cusID;
    bool hasFinishedCutting;
};

typedef struct chair
{
    struct customerData *data; /*the customer who sits on the chair*/
    /*If nobody sits on it, then customerData = null*/
    int seqNumber;/*The chair's sequence number of all chairs*/
} Chair;

Chair waitingChairs[NUM_CHAIRS];    /*Chairs for waiting area*/
int availableChairs = NUM_CHAIRS;   /*Number of available waiting chairs*/

int totalServedCustomers = 0;       /*number of customers that have been served by barbers*/
int nextID = 1;     /* ID for customer */

int nextCut = 0;    /*  Point to the chair which next served customer sits on  */
int nextSit = 0;    /*  Point to the chair which will be sat when next customer comes */

/* Global Time */
int cus_perTime[TIME_RANGE];
int currentTime = 0;

/*For poisson distribution*/
float mean;
int timeRange; //1 period will have how many time unit
int num_customer; //number (for poisson distribution) to create customers
int realNum_customer = 0;

void showWhoSitOnChair()
{
    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout<<"Waiting Chairs:";
    for(int i=0; i<NUM_CHAIRS; i++)
    {
        if(waitingChairs[i].data != nullptr){
            isSit[i] = true;
            cout << waitingChairs[i].data->cusID << " ";
        }
        else{
            isSit[i] = false;
            cout << "0 ";
        }
    }
    cout << endl;
    print();
    dispatch_semaphore_signal(ioMutex); // Release waiting
}

void cutHair(int barberID, Chair wChair)
{
    isBusy[barberID-1] = true;

    gui->changeBarberMode(barberID,true);

    dispatch_semaphore_wait(cusMutex, DISPATCH_TIME_FOREVER);
    ++totalServedCustomers;
    dispatch_semaphore_signal(barMutex);

    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout<<"total:"<<totalServedCustomers<<"/"<<realNum_customer<<endl;
    dispatch_semaphore_signal(ioMutex); // Release waiting

    dispatch_semaphore_signal(cusMutex);

    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "(B) Barber " << barberID <<" is cutting Customer No." << wChair.data->cusID << "'s hair !"<<endl;
    //cout << "(At chair No." << wChair.seqNumber << ")" << endl;
    cutting[barberID-1] = wChair.data->cusID;
    print();
    dispatch_semaphore_signal(ioMutex); // Release waiting

    waitingChairs[wChair.seqNumber].data = nullptr;
    isSit[wChair.seqNumber] = false;

    for(long long i=0; i<200000000; i++); //Cut hair time
    isBusy[barberID-1] = false;
    //gui->changeBarberMode(barberID,false);


    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "(B)Barber " << barberID <<" just finished cutting Customer No." << wChair.data->cusID << "'s hair !" <<endl<<endl;;
    dispatch_semaphore_signal(ioMutex); // Release waiting

    wChair.data->hasFinishedCutting = true;
}

void *barberThread(void* arg)
{
    int *pID = (int*)arg;
    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "This is Barber No." << *pID << endl;
    dispatch_semaphore_signal(ioMutex); // Release waiting

    while(1)
    {
        dispatch_semaphore_wait(cusMutex, DISPATCH_TIME_FOREVER);
        //		dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
        //		cout<<"\n----- "<<totalServedCustomers<<" / "<<cus_perTime[currentTime]<<" -----\n";
        //		dispatch_semaphore_signal(ioMutex);
        if(totalServedCustomers >= realNum_customer){
            dispatch_semaphore_signal(cusMutex);
            //			dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
            //			cout<<"\n@@ Barber "<<*pID<<" e04!!\n\n";
            //			dispatch_semaphore_signal(ioMutex);
            break;
        }
        /*
         if(totalServedCustomers >= cus_perTime[currentTime] && totalServedCustomers != realNum_customer){
         dispatch_semaphore_signal(cusMutex);
         continue;
         }
         */
        dispatch_semaphore_signal(cusMutex);

        dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
        cout<<"\nBarber "<<*pID<<" is free!!\n\n";
        dispatch_semaphore_signal(ioMutex);

    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    print();
    dispatch_semaphore_signal(ioMutex); // Release waiting

        dispatch_semaphore_wait(barMutex,DISPATCH_TIME_FOREVER);
        //				dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
        //				cout<<"\n----- "<<totalServedCustomers<<" / "<<cus_perTime[currentTime]<<" -----\n";
        //				dispatch_semaphore_signal(ioMutex);
        if(totalServedCustomers < realNum_customer){
            //			dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
            //			cout<<"\n@1 Barber "<<*pID<<" cus\n\n";
            //			dispatch_semaphore_signal(ioMutex);
            dispatch_semaphore_wait(customers, DISPATCH_TIME_FOREVER); // Try to acquire a customer.

            //Go to sleep if no customers
            //			dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
            //			cout<<"\n@2 Barber "<<*pID<<" mtx\n\n";
            //			dispatch_semaphore_signal(ioMutex);
            dispatch_semaphore_wait(Mutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting

            //When a barber is waken -> wants to modify # of available chairs
            //			dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
            //			cout<<"\n@2 Barber "<<*pID<<" barbers\n\n";
            //			dispatch_semaphore_signal(ioMutex);
            dispatch_semaphore_signal(barbers);  // The barber is now ready to cut hair


            int nowCut = nextCut;
            nextCut = (nextCut+1) % NUM_CHAIRS;
            availableChairs++;

            dispatch_semaphore_signal(Mutex); // Release waiting
            cutHair(*pID, waitingChairs[nowCut]); //pick the customer which counter point
        }
        else{
            dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
            cout<<"\n@2 Barber "<<*pID<<" arrrrr\n\n";
            dispatch_semaphore_signal(ioMutex);
            dispatch_semaphore_signal(barMutex);
        }
    }
    pthread_exit(0);
}


void waitForHairCut(struct customerData *a)
{
    while(a->hasFinishedCutting == false);
}

void *customerThread(void* arg)
{
    struct customerData *data = (struct customerData*)arg;
    dispatch_semaphore_wait(Mutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting

    if( availableChairs == 0 )
    {
        dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
        cout << "There is no available chair. Customer No." << data->cusID << " is leaving!" << endl;
        comeCus[data->cusID-1] = false;
        dispatch_semaphore_signal(ioMutex); // Release waiting

        dispatch_semaphore_wait(cusMutex, DISPATCH_TIME_FOREVER);
        --realNum_customer;
        --cus_perTime[currentTime];
        dispatch_semaphore_signal(cusMutex);

        dispatch_semaphore_signal(Mutex);
        pthread_exit(0);
    }
    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "Customer No." << data->cusID << " is sitting on chair " << nextSit << "." << endl;
    comeCus[data->cusID-1] = false;
    seat[nextSit] = data->cusID;
    dispatch_semaphore_signal(ioMutex); // Release waiting

    waitingChairs[nextSit].data = data;
    nextSit = (nextSit+1) % NUM_CHAIRS;
    availableChairs--;
    showWhoSitOnChair();

    dispatch_semaphore_signal(customers); // Wake up a barber (if needed)
    dispatch_semaphore_signal(Mutex); // Release waiting
    dispatch_semaphore_wait(barbers, DISPATCH_TIME_FOREVER); // Go to sleep if number of available barbers is 0
    waitForHairCut(data);

    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "(C)Customer No." << data->cusID <<" just finished his haircut!"<<endl;
    dispatch_semaphore_signal(ioMutex); // Release waiting
    pthread_exit(0);
}

int *possionDistribution(float mean, int range, int num_period)
{
    const int NUM_TIMES = num_period;
    default_random_engine generator;
    poisson_distribution<int> distribution(mean);

    int *frequenceArray = new int[range];

    for(int i=0; i<range; i++)
        frequenceArray[i] = 0;

    for(int i=0; i<NUM_TIMES; i++)
    {
        int number = distribution(generator);
        if(number < range)
            frequenceArray[number]++;
    }

    realNum_customer = 0;

    for(int i=0; i<range; i++)
    {
        dispatch_semaphore_wait(cusMutex,DISPATCH_TIME_FOREVER);

        dispatch_semaphore_wait(ioMutex,DISPATCH_TIME_FOREVER);
        cout << i << " : " << frequenceArray[i] <<endl;
        dispatch_semaphore_signal(ioMutex);

        realNum_customer += frequenceArray[i];
        cus_perTime[i] = realNum_customer;
        dispatch_semaphore_signal(cusMutex);
    }
    dispatch_semaphore_wait(ioMutex,DISPATCH_TIME_FOREVER);
    cout << "Sum : " << realNum_customer << endl << endl;
    dispatch_semaphore_signal(ioMutex);

    return frequenceArray;
}

void createCustomers(int timeRange,int num_customer,float mean,int* cusArray)
{
    pthread_t cus[num_customer];
    int cusTH = 0;      //this is n-th customer. (0 represent the first customer)
    struct customerData cusData[num_customer];

    for(currentTime=0; currentTime<timeRange; currentTime++)
    {
        dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
        cout<<endl<<"*****TIME:"<<currentTime<<endl;
        dispatch_semaphore_signal(ioMutex); // Release waiting
        for(int j=0; j<cusArray[currentTime]; j++)
        {
            cusData[cusTH].cusID = nextID;
            cusData[cusTH].hasFinishedCutting = false;

            dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
            cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< currentTime << ")"<<endl;
            comeCus[cusData[cusTH].cusID-1] = true;
            print();
            dispatch_semaphore_signal(ioMutex); // Release waiting

            pthread_create(&cus[cusTH], NULL, &customerThread, (void*)&cusData[cusTH]);

            cusTH ++;
            nextID ++;
            usleep(10);     // avoid create earlier but execute thread laterly
        }
        usleep(1000000);    // next time unit
    }

    for(int i=0; i<num_customer; i++)
    {
        pthread_join(cus[i], NULL);
        //cout<<"////pthread_cus"<<endl;
    }

    //	cout<<"////pthread_cus_exit"<<endl;
}


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

    gui = this;
}

void test(){
    gui->changeBarberMode(1,true);

}

void MainWindow::mouseMoveEvent ( QMouseEvent * event )
{
}


void MainWindow::mouseReleaseEvent ( QMouseEvent * event )
{
  if(event->button() == Qt::LeftButton)
  {
      test();

      mean = MEAN;
      timeRange = TIME_RANGE;
      num_customer = NUM_CUSTOMER;

      for(int i=0;i<NUM_BARBERS;i++){
          cutting[i] = -1;
          isBusy[i] = false;
      }
      for(int i=0;i<NUM_CUSTOMER;i++){
          comeCus[i] = false;
      }


      for(int i=0; i<NUM_CHAIRS; i++)  // fill the number of tn of all waiting chair
          waitingChairs[i].seqNumber = i;

      pthread_t bar[NUM_BARBERS];
      int barberID[NUM_BARBERS];

      int *cusArray = possionDistribution(mean, timeRange, num_customer); //Use p_s create

      for(int i=0; i<NUM_BARBERS; i++)
      {
          barberID[i] = i+1; // fill the barID
          pthread_create(&bar[i], NULL, &barberThread, (void*)&barberID[i]);  // create all barber thread
      }
      createCustomers(timeRange,num_customer,mean,cusArray);

      for(int i=0; i<NUM_BARBERS; i++)
      {
          cout<<"////pthread_bar"<<i<<endl;
          pthread_join(bar[i], NULL);
      }
      cout<<"////pthread_bar_exit"<<endl;

      dispatch_release(customers);
      dispatch_release(barbers);
      dispatch_release(Mutex);
      dispatch_release(ioMutex);
      dispatch_release(cusMutex);
      dispatch_release(barMutex);
      print();

      cout<<endl<<"All customers finish their haircuts!"<<endl;

  }
}

QImage *busyImage = new QImage( "/Users/HSUAN/os2016_group14/proj_gui/pic/barber_busy.png" ) ;
QImage *sleepImage = new QImage( "/Users/HSUAN/os2016_group14/proj_gui/pic/barber_sleep.png" ) ;

void MainWindow::changeBarberMode(int i,bool isBusy){
#if 0
    app.processEvents();
#endif
    if(isBusy){
        cout<<"hi\n";
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
