#include <fcntl.h>
#include <iostream>
#include <dispatch/dispatch.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <random>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

using namespace std;

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
//int barbers=NUM_BARBERS,customers=0,Mutex=1,ioMutex=1,cusMutex=1;
/*
barbers = dispatch_semaphore_create(NUM_BARBERS);
customers = dispatch_semaphore_create(0);
Mutex = dispatch_semaphore_create(1);
ioMutex = dispatch_semaphore_create(1);
cusMutex = dispatch_semaphore_create(1);
*/
/*
int dispatch_semaphore_wait(int sem){
	while(sem<=0);
	sem--;
	return 0;
}

int dispatch_semaphore_signal(int sem){
	sem++;
	return 0;
}
*/
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
        if(waitingChairs[i].data != nullptr)
            cout << waitingChairs[i].data->cusID << " ";
        else cout << "0 ";
    }
    cout << endl;
    dispatch_semaphore_signal(ioMutex); // Release waiting
}

void cutHair(int barberID, Chair wChair)
{
    dispatch_semaphore_wait(cusMutex, DISPATCH_TIME_FOREVER);
    totalServedCustomers++;
    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout<<"total:"<<totalServedCustomers<<"/"<<realNum_customer<<endl;
    dispatch_semaphore_signal(ioMutex); // Release waiting
    dispatch_semaphore_signal(cusMutex);

    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "(B) Barber " << barberID <<" is cutting Customer No." << wChair.data->cusID << "'s hair !"<<endl;
    //cout << "(At chair No." << wChair.seqNumber << ")" << endl;
    dispatch_semaphore_signal(ioMutex); // Release waiting

    waitingChairs[wChair.seqNumber].data = nullptr;

    for(long long i=0; i<200000000; i++); //Cut hair time

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

    while(totalServedCustomers < realNum_customer)
    {
		dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER);
		cout<<"\nThis is Barber "<<*pID<<" cutting!!\n\n";
		dispatch_semaphore_signal(ioMutex);
        dispatch_semaphore_wait(customers, DISPATCH_TIME_FOREVER); // Try to acquire a customer.
        //Go to sleep if no customers
        dispatch_semaphore_wait(Mutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
        //When a barber is waken -> wants to modify # of available chairs
        dispatch_semaphore_signal(barbers);  // The barber is now ready to cut hair

        int nowCut = nextCut;
        nextCut = (nextCut+1) % NUM_CHAIRS;
        availableChairs++;

        dispatch_semaphore_signal(Mutex); // Release waiting
        cutHair(*pID, waitingChairs[nowCut]); //pick the customer which counter point
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
        dispatch_semaphore_signal(ioMutex); // Release waiting

        dispatch_semaphore_wait(cusMutex, DISPATCH_TIME_FOREVER);
        realNum_customer--;
        dispatch_semaphore_signal(cusMutex);

        dispatch_semaphore_signal(Mutex);
        pthread_exit(0);
    }
    dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
    cout << "Customer No." << data->cusID << " is sitting on chair " << nextSit << "." << endl;
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
        cout << i << " : " << frequenceArray[i] <<endl;
        realNum_customer += frequenceArray[i];
    }
    cout << "Sum : " << realNum_customer << endl << endl;
    dispatch_semaphore_signal(ioMutex);


    return frequenceArray;
}

void createCustomers(int timeRange,int num_customer,float mean)
{

    pthread_t cus[num_customer];
    int cusTH = 0;      //this is n-th customer. (0 represent the first customer)

    struct customerData cusData[num_customer];

    int *cusArray = possionDistribution(mean, timeRange, num_customer); //Use p_s create

    for(int i=0; i<timeRange; i++)
    {
        dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
        cout<<endl<<"*****TIME:"<<i<<endl;
        dispatch_semaphore_signal(ioMutex); // Release waiting
        for(int j=0; j<cusArray[i]; j++)
        {
            cusData[cusTH].cusID = nextID;
            cusData[cusTH].hasFinishedCutting = false;

            dispatch_semaphore_wait(ioMutex, DISPATCH_TIME_FOREVER); // Acquire access to waiting
            cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< i << ")"<<endl;
            dispatch_semaphore_signal(ioMutex); // Release waiting

            pthread_create(&cus[cusTH], NULL, customerThread, (void*)&cusData[cusTH]);

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

    cout<<"////pthread_cus_exit"<<endl;
}

int main()
{

    cout<<"Enter mean number (for poisson distribution):";
    cin>>mean;
    cout<<"Enter the time range (sec) that you want to test:";
    cin>>timeRange;
    cout<<"Enter number (for poisson distribution) to create customers:";
    cin>>num_customer;


/*
    if( (customers = sem_open("a",O_CREAT,0644,1)) == SEM_FAILED){
        cout << "Fail" << endl;
    }
    if( (barbers = sem_open("b",O_CREAT,0644, NUM_BARBERS) )== SEM_FAILED){
        cout << "Fail" << endl;
    }
    if((Mutex = sem_open("c",O_CREAT,0644, 1) )== SEM_FAILED){
        cout << "Fail" << endl;
    }
    if( (ioMutex = sem_open("d",O_CREAT,0644 ,1))==  SEM_FAILED){
        cout << "Fail" << endl;
    }

    if( (cusMutex = sem_open("e",O_CREAT,0644 ,1))== SEM_FAILED){
        cout << "Fail" << endl;
    }     
	
    int value = 0;
    sem_getvalue(Mutex, &value);
    cout << "Mutex = "<< value << endl;
    sem_getvalue(barbers, &value);
    cout << "Barber = "<< value << endl;
    sem_getvalue(customers, &value);
    cout << "Customer = "<< value << endl;
*/


    for(int i=0; i<NUM_CHAIRS; i++)  // fill the number of tn of all waiting chair
        waitingChairs[i].seqNumber = i;

    pthread_t bar[NUM_BARBERS];
    int barberID[NUM_BARBERS];

    for(int i=0; i<NUM_BARBERS; i++)
    {
        barberID[i] = i+1; // fill the barID
        pthread_create(&bar[i], NULL, barberThread, (void*)&barberID[i]);  // create all barber thread
    }

    createCustomers(timeRange,num_customer,mean);


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


    cout<<endl<<"All customers finish their haircuts!"<<endl;
    return 0;
}
