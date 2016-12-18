#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <random>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

#define MEAN 5
#define TIME_RANGE 10
#define NUM_CUSTOMER 20

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
sem_t barbers;      /*Number of barbers waiting for customers*/
sem_t customers;    /*Number of customers waiting for service*/
sem_t Mutex;        /*Mutex used for mutual exclusion*/
sem_t ioMutex;      /*Mutex used for input and output*/
sem_t cusMutex;     /*Mutex used for change totalServedCustomers*/
sem_t barMutex;     /*Mutex used for one check per time*/

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
    sem_wait(&ioMutex); // Acquire access to waiting
    cout<<"Waiting Chairs:";
    for(int i=0; i<NUM_CHAIRS; i++)
    {
        if(waitingChairs[i].data != nullptr)
            cout << waitingChairs[i].data->cusID << " ";
        else cout << "0 ";
    }
    cout << endl;
    sem_post(&ioMutex); // Release waiting
}

void cutHair(int barberID, Chair wChair)
{
	sem_wait(&cusMutex);
	++totalServedCustomers;
	sem_post(&barMutex);

	sem_wait(&ioMutex); // Acquire access to waiting
	cout<<"total:"<<totalServedCustomers<<"/"<<realNum_customer<<endl;
	sem_post(&ioMutex); // Release waiting

	sem_post(&cusMutex);

	sem_wait(&ioMutex); // Acquire access to waiting
	cout << "(B) Barber " << barberID <<" is cutting Customer No." << wChair.data->cusID << "'s hair !"<<endl;
	sem_post(&ioMutex); // Release waiting

	waitingChairs[wChair.seqNumber].data = nullptr;

	for(long long i=0; i<200000000; i++); //Cut hair time

	sem_wait(&ioMutex); // Acquire access to waiting
	cout << "(B)Barber " << barberID <<" just finished cutting Customer No." << wChair.data->cusID << "'s hair !" <<endl<<endl;;
	sem_post(&ioMutex); // Release waiting

	wChair.data->hasFinishedCutting = true;
}

void *barberThread(void* arg)
{
	int *pID = (int*)arg;
	sem_wait(&ioMutex); // Acquire access to waiting
	cout << "This is Barber No." << *pID << endl;
	sem_post(&ioMutex); // Release waiting

	while(1)
	{
        /*
        //sem_wait(&cusMutex);
		if(totalServedCustomers >= realNum_customer){
            //sem_post(&cusMutex);
			break;
		}

		if(totalServedCustomers >= cus_perTime[currentTime] && totalServedCustomers < realNum_customer){
			//sem_post(&cusMutex);
			cout<<"Wait"<<endl;
			continue;
		}
        //sem_post(&cusMutex);
        */

		sem_wait(&barMutex);
        sem_wait(&cusMutex);
		if(totalServedCustomers < realNum_customer){
            sem_post(&cusMutex);
			sem_wait(&customers); // Try to acquire a customer.
			//Go to sleep if no customers
			sem_wait(&Mutex); // Acquire access to waiting
			//When a barber is waken -> wants to modify # of available chairs
			sem_post(&barbers);  // The barber is now ready to cut hair

			int nowCut = nextCut;
			nextCut = (nextCut+1) % NUM_CHAIRS;
			availableChairs++;

			sem_post(&Mutex); // Release waiting
			cutHair(*pID, waitingChairs[nowCut]); //pick the customer which counter point
		}
		else{
			sem_post(&barMutex);
			sem_post(&cusMutex);
			break;
		}
	}
}

void waitForHairCut(struct customerData *a)
{
	while(a->hasFinishedCutting == false);
}

void *customerThread(void* arg)
{
    struct customerData *data = (struct customerData*)arg;
    sem_wait(&Mutex); // Acquire access to waiting
    if( availableChairs == 0 )
    {
        sem_wait(&ioMutex); // Acquire access to waiting
        cout << "There is no available chair. Customer No." << data->cusID << " is leaving!" << endl;
        sem_post(&ioMutex); // Release waiting

        sem_wait(&cusMutex);
        --realNum_customer;
        //--cus_perTime[currentTime];
        sem_post(&cusMutex);

        sem_post(&Mutex);
        pthread_exit(0);
    }
    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "Customer No." << data->cusID << " is sitting on chair " << nextSit << "." << endl;
    sem_post(&ioMutex); // Release waiting

    waitingChairs[nextSit].data = data;
    nextSit = (nextSit+1) % NUM_CHAIRS;
    availableChairs--;
    showWhoSitOnChair();

    sem_post(&customers); // Wake up a barber (if needed)
    sem_post(&Mutex); // Release waiting
    sem_wait(&barbers); // Go to sleep if number of available barbers is 0
    waitForHairCut(data);

    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "(C)Customer No." << data->cusID <<" just finished his haircut!"<<endl;
    sem_post(&ioMutex); // Release waiting

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
		sem_wait(&cusMutex);

		sem_wait(&ioMutex);
		cout << i << " : " << frequenceArray[i] <<endl;
		sem_post(&ioMutex);

		realNum_customer += frequenceArray[i];
		//cus_perTime[i] = realNum_customer;
		sem_post(&cusMutex);
	}
	sem_wait(&ioMutex);
	cout << "Sum : " << realNum_customer << endl << endl;
	sem_post(&ioMutex);

	return frequenceArray;
}

void createCustomers(int timeRange,int num_customer,float mean,int* cusArray)
{
	pthread_t cus[num_customer];
	int cusTH = 0;      //this is n-th customer. (0 represent the first customer)
	struct customerData cusData[num_customer];

	for(currentTime=0; currentTime<timeRange; currentTime++)
	{
		sem_wait(&ioMutex);
		cout<<"*****TIME:"<<currentTime<<endl;
		sem_post(&ioMutex); // Release waiting
		for(int j=0; j<cusArray[currentTime]; j++)
		{
			cusData[cusTH].cusID = nextID;
			cusData[cusTH].hasFinishedCutting = false;

			sem_wait(&ioMutex);
			cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< currentTime << ")"<<endl;
			sem_post(&ioMutex); // Release waiting

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

int main()
{

	/*
	   cout<<"Enter mean number (for poisson distribution):";
	   cin>>mean;
	   cout<<"Enter the time range (sec) that you want to test:";
	   cin>>timeRange;
	   cout<<"Enter number (for poisson distribution) to create customers:";
	   cin>>num_customer;
	 */

	mean = MEAN;
	timeRange = TIME_RANGE;
	num_customer = NUM_CUSTOMER;

    sem_init(&customers, 0, 0); // at first, no customer
    sem_init(&barbers, 0, NUM_BARBERS);
    sem_init(&Mutex, 0, 1);
    sem_init(&ioMutex, 0, 1);
    sem_init(&cusMutex, 0, 1);
    sem_init(&barMutex, 0, 1);


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

    sem_destroy(&barbers);
    sem_destroy(&customers);
    sem_destroy(&Mutex);
    sem_destroy(&ioMutex);
    sem_destroy(&cusMutex);
    sem_destroy(&barMutex);

	cout<<endl<<"All customers finish their haircuts!"<<endl;
	return 0;
}
