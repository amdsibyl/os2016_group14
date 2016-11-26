#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <windows.h>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

using namespace std;

//modified from pseudocode_1.cpp

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
/*Number of barbers waiting for customers*/
sem_t barbers;

/*Number of customers waiting for service*/
sem_t customers;

/*Mutex used for mutual exclusion*/
sem_t mutex;

/*Number of customers who are waiting*/
int waitingCustomers = 0;

/*Empty chairs for waiting area*/
int availableWaitingChairs = NUM_CHAIRS;

/*Number of sleeping barbers*/
int sleepingBarbers = 0;

//int loopTime = 5;/*For test*/

class Barber
{
private:
    int id;
    bool isBusy = false;
public:
    void Barber(int id)
    {
        this->id = id;
    }
    void check();
    void cutHair()
    {
        cout<<"Barber " + id + " is busy now!"<<endl;
        try
        {
            sleep(5000);//sleep for 5s
        }
        catch(InterruptedException ex) {}
    }
};

void Barber::check()
{
    while(1)
    {
        customers.down(); // Go to sleep if no customers
        mutex.down(); // Acquire access to waiting
        waiting = waiting - 1; // Decrement count of waiting customers
        barbers.up();  // One barber is now ready to cut hair
        mutex.up(); // Release waiting
        cutHair(); // Noncritical region
    }
}

class Customer
{
private:
    int id;
    bool isBusy = false;
public:
    void Customer(int id)
    {
        this->id = id;
    }
    void check();
    void getHairCut()
    {
        cout<<"Customer " + id + " is getting his/her hair cut!"<<endl;
        try
        {
            sleep(5000);//sleep for 5s
        }
        catch(InterruptedException ex) {}
    }
};

void Customer::check()
{
    mutex.down(); // Acquire access to waiting
    if(waiting ‹ CHAIRS)
    {
        waiting = waiting + 1; // Increment count of waiting customers
        customers.up(); // Wake up barber if needed
        mutex.up(); // Release waiting
        barbers.down(); // Go to sleep if number of available barbers is 0
        get_haircut(); // Noncritical region
    }
    else
    {
        mutex.up(); // Shop is full do not wait
    }

}

void *barberThread(void*)
{

    /*
    	if no customer -> go to sleep
    	if have -> pick one
    */

    /*Cut hair*/
    sem_post(&barber);


    pthread_exit(0);
}

//threads' names need to be modified.
void *customerThread(void*)
{
    if(sleepingBarbers  != 0 )
    {

        /* Wake up a barber */
        sem_post(&customers);
        sem_wait(&barbers);

    }
    else if(availableWaitingChairs == 0)
        pthread_exit(0);
    else
    {
        sem_wait(&mutex);
        availableWaitingChairs--;
        sem_post(&mutex);

        sem_post(&customer);
        sem_wait(&barber);
    }
}

void createCustomer()
{
    while(true)
    {
        /* sleep random time */
        /* create customer thread*/
    }
}
int main()
{
    sem_init(&barbers, 0, NUM_BARBERS);
    sem_init(&customers, 0, 0);
    sem_init(&mutex, 0, 1);

    pthread_t tid[NUM_BARBERS];
    for(int i=0 ; i<NUM_BARBERS; i++)
    {
        pthread_create(&tid[i], NULL, barbers, NULL);
    }
    for(int i=0 ; i<NUM_BARBERS; i++)
    {
        pthread_join(tid[i]);
    }
}
