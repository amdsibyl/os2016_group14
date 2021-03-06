#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

using namespace std;

//modified from pseudocode_2.cpp

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

sem_t barbers;/*Number of barbers waiting for customers*/
sem_t customers;/*Number of customers waiting for service*/
sem_t mutex;/*Mutex used for mutual exclusion*/


typedef struct chair
{
    int customerID;/*Number of the customer who sits on the chair*/
    /*If nobody sits on it, then customerID = 0*/
    int seqNumber;/*The chair's sequence number of all chairs*/
} Chair;

/*Chairs for waiting area*/
Chair waitingChairs[NUM_CHAIRS];

/*Number of available waiting chairs*/
int availableChairs = NUM_CHAIRS;

/*Number of sleeping barbers*/
int sleepingBarbers = 0;

int nextID = 1;  /* ID for customer */

int nextCut = 0;    /*  Point to the chair which next served customer sits on  */
int nextSit = 0;    /*  Point to the chair which will be sat when next customer comes */


void showWhoSitOnChair()
{
    cout<<"Waiting Chairs:";
    for(int i=0; i<NUM_CHAIRS; i++)
        cout << waitingChairs[i].customerID << " ";
    cout << endl;
}

void cutHair(int barberID, Chair wChair)
{
    cout << "Barber " << barberID <<" is cutting Customer No." << wChair.customerID << "'s hair !"<<endl;
    //cout << "(At chair No." << wChair.seqNumber << ")" << endl;
    nextCut = (nextCut+1) % NUM_CHAIRS;
    waitingChairs[wChair.seqNumber].customerID = 0;
    availableChairs++;
    usleep(5000000);//sleep for 5s
    //for(long i=0; i<100000000; i++);
    cout << "Barber " << barberID <<" just finished cutting Customer No." << wChair.customerID << "!" <<endl<<endl;;
}
/*
void getHairCut(int id)
{
    //usleep(5000000);
    usleep(100);
    cout<<"Customer No."<<id<<" is getting his/her hair cut."<<endl;
    usleep(499900);
}
*/
void getHairCut()
{
    usleep(4999000);
}
void *barberThread(void* arg)
{
    int *pID = (int*)arg;
    cout << "This is Barber No." << *pID << endl;
    while(true)
    {
        sem_wait(&customers); // Try to acquire a customer.
        //Go to sleep if no customers

        sem_wait(&mutex); // Acquire access to waiting
        //When a barber is waken -> wants to modify # of available chairs

        sem_post(&barbers);  // The barber is now ready to cut hair
        sem_post(&mutex); // Release waiting
        //don't need to lock on the chair anymore

        cutHair(*pID, waitingChairs[nextCut]); //pick the customer which counter point
    }
}

void *customerThread(void* arg)
{
    int *pID = (int*)arg;
    sem_wait(&mutex); // Acquire access to waiting
    //execute a DOWN on mutex before entering critical section

    if( availableChairs == 0 )
    {
        cout << "There is no available chair. Customer No." << *pID << " is leaving!" << endl;
        sem_post(&mutex);
        pthread_exit(0);
    }

    cout << "Customer No." << *pID << " is sitting on chair " << nextSit << "." << endl;
    waitingChairs[nextSit].customerID = *pID;
    nextSit = (nextSit+1) % NUM_CHAIRS;
    availableChairs--;
    showWhoSitOnChair();

    sem_post(&customers); // Wake up a barber (if needed)
    sem_post(&mutex); // Release waiting
    //execute an UP on mutex when leaving critical section

    sem_wait(&barbers); // Go to sleep if number of available barbers is 0
    //getHairCut(*pID);
    getHairCut();
}

void createCustomers()
{
    int randomNum = 10;

    pthread_t cus[randomNum];
    int customerID[randomNum];

    for(int i=0; i<randomNum; i++)
    {
        customerID[i] = i+1;
        cout <<endl<< "Create Customer No."<< customerID[i] <<"."<< endl;
        pthread_create(&cus[i], NULL, customerThread, (void*)&customerID[i]);
        usleep(100000);
    }
}

int main()
{
    sem_init(&customers, 0, 0); // at first, no customer
    sem_init(&barbers, 0, NUM_BARBERS);
    sem_init(&mutex, 0, 1);

    for(int i=0; i<NUM_CHAIRS; i++)  // fill the number of tn of all waiting chair
        waitingChairs[i].seqNumber = i;

    pthread_t bar[NUM_BARBERS];
    int barberID[NUM_BARBERS];

    for(int i=0; i<NUM_BARBERS; i++)
    {
        barberID[i] = i+1; // fill the barID
        pthread_create(&bar[i], NULL, barberThread, (void*)&barberID[i]);  // create all barber thread
    }

    createCustomers();

    for(int i=0; i<NUM_BARBERS; i++)
        pthread_join(bar[i], NULL);

    return 0;
}
