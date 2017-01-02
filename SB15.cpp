#include <iostream>
#include <semaphore.h>
//#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <random>
#include <thread>
//#include <semaphore.h>
#include <mutex>
#include <condition_variable>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

#define MEAN 5
#define TIME_RANGE 10
#define NUM_CUSTOMER 10

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


class Semaphore {
    public:
        Semaphore(int value=1): count{value}, wakeups{0} {}

        void wait(){
            std::unique_lock<std::mutex> lock{mutex};
            if (--count<0) { // count is not enough ?
                condition.wait(lock, [&]()->bool{ return wakeups>0;}); // suspend and wait ...
                --wakeups;  // ok, me wakeup !
            }
        }
        void signal(){
            std::lock_guard<std::mutex> lock{mutex};
            if(++count<=0) { // have some thread suspended ?
                ++wakeups;
                condition.notify_one(); // notify one !
            }
        }

    private:
        int count;
        int wakeups;
        std::mutex mutex;
        std::condition_variable condition;
};

 /*Shared data*/
//sem_t barbers;      /*Number of barbers waiting for customers*/
//sem_t customers;    /*Number of customers waiting for service*/
//sem_t Mutex;        /*Mutex used for mutual exclusion*/
//sem_t ioMutex;      /*Mutex used for input and output*/
//sem_t cusMutex;     /*Mutex used for change totalServedCustomers*/
//sem_t barMutex;     /*Mutex used for one check per time*/


Semaphore barbers(NUM_BARBERS);
Semaphore customers(0);
mutex Mutex,ioMutex,cusMutex,barMutex;


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
    ioMutex.lock(); // Acquire access to waiting
    cout<<"Waiting Chairs:";
    for(int i=0; i<NUM_CHAIRS; i++)
    {
        if(waitingChairs[i].data != nullptr)
            cout << waitingChairs[i].data->cusID << " ";
        else cout << "0 ";
    }
    cout << endl;
    ioMutex.unlock(); // Release waiting
}

void cutHair(int barberID, Chair wChair)
{
	cusMutex.lock();
	++totalServedCustomers;
	barMutex.unlock();

	ioMutex.lock(); // Acquire access to waiting
	cout<<"total:"<<totalServedCustomers<<"/"<<realNum_customer<<endl;
	ioMutex.unlock(); // Release waiting

	cusMutex.unlock();

	ioMutex.lock(); // Acquire access to waiting
	cout << "(B) Barber " << barberID <<" is cutting Customer No." << wChair.data->cusID << "'s hair !"<<endl;
	ioMutex.unlock(); // Release waiting

	waitingChairs[wChair.seqNumber].data = nullptr;

	for(long long i=0; i<200000000; i++); //Cut hair time

	ioMutex.lock(); // Acquire access to waiting
	cout << "(B)Barber " << barberID <<" just finished cutting Customer No." << wChair.data->cusID << "'s hair !" <<endl<<endl;;
	ioMutex.unlock(); // Release waiting

	wChair.data->hasFinishedCutting = true;
}

void *barberThread(void* arg)
{
	int *pID = (int*)arg;
	ioMutex.lock(); // Acquire access to waiting
	cout << "This is Barber No." << *pID << endl;
	ioMutex.unlock(); // Release waiting

	while(1)
	{
        /*
        //cusMutex.lock();
		if(totalServedCustomers >= realNum_customer){
            //cusMutex.unlock();
			break;
		}

		if(totalServedCustomers >= cus_perTime[currentTime] && totalServedCustomers < realNum_customer){
			//cusMutex.unlock();
			cout<<"Wait"<<endl;
			continue;
		}
        //cusMutex.unlock();
        */

		barMutex.lock();
        cusMutex.lock();
		if(totalServedCustomers < realNum_customer){
            cusMutex.unlock();
			customers.wait(); // Try to acquire a customer.
			//Go to sleep if no customers
			Mutex.lock(); // Acquire access to waiting
			//When a barber is waken -> wants to modify # of available chairs
			barbers.signal();  // The barber is now ready to cut hair

			int nowCut = nextCut;
			nextCut = (nextCut+1) % NUM_CHAIRS;
			availableChairs++;

			Mutex.unlock(); // Release waiting
			cutHair(*pID, waitingChairs[nowCut]); //pick the customer which counter point
		}
		else{
			barMutex.unlock();
			cusMutex.unlock();
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
    Mutex.lock(); // Acquire access to waiting
    if( availableChairs == 0 )
    {
        ioMutex.lock(); // Acquire access to waiting
        cout << "There is no available chair. Customer No." << data->cusID << " is leaving!" << endl;
        ioMutex.unlock(); // Release waiting

        cusMutex.lock();
        --realNum_customer;
        //--cus_perTime[currentTime];
        cusMutex.unlock();

        Mutex.unlock();
        pthread_exit(0);
    }
    ioMutex.lock(); // Acquire access to waiting
    cout << "Customer No." << data->cusID << " is sitting on chair " << nextSit << "." << endl;
    ioMutex.unlock(); // Release waiting

    waitingChairs[nextSit].data = data;
    nextSit = (nextSit+1) % NUM_CHAIRS;
    availableChairs--;
    showWhoSitOnChair();

    customers.signal(); // Wake up a barber (if needed)
    Mutex.unlock(); // Release waiting
    barbers.wait(); // Go to sleep if number of available barbers is 0
    waitForHairCut(data);

    ioMutex.lock(); // Acquire access to waiting
    cout << "(C)Customer No." << data->cusID <<" just finished his haircut!"<<endl;
    ioMutex.unlock(); // Release waiting

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
		cusMutex.lock();

		ioMutex.lock();
		cout << i << " : " << frequenceArray[i] <<endl;
		ioMutex.unlock();

		realNum_customer += frequenceArray[i];
		//cus_perTime[i] = realNum_customer;
		cusMutex.unlock();
	}
	ioMutex.lock();
	cout << "Sum : " << realNum_customer << endl << endl;
	ioMutex.unlock();

	return frequenceArray;
}

void createCustomers(int timeRange,int num_customer,float mean,int* cusArray)
{
	//pthread_t cus[num_customer];
	thread cus[num_customer];

	int cusTH = 0;      //this is n-th customer. (0 represent the first customer)
	struct customerData cusData[num_customer];

	for(currentTime=0; currentTime<timeRange; currentTime++)
	{
		ioMutex.lock();
		cout<<"*****TIME:"<<currentTime<<endl;
		ioMutex.unlock(); // Release waiting
		for(int j=0; j<cusArray[currentTime]; j++)
		{
			cusData[cusTH].cusID = nextID;
			cusData[cusTH].hasFinishedCutting = false;

			ioMutex.lock();
			cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< currentTime << ")"<<endl;
			ioMutex.unlock(); // Release waiting

			//pthread_create(&cus[cusTH], NULL, &customerThread, (void*)&cusData[cusTH]);
            cus[cusTH] = thread(&customerThread, (void*)&cusData[cusTH]);

			cusTH ++;
			nextID ++;
			usleep(10);     // avoid create earlier but execute thread laterly
		}
		usleep(1000000);    // next time unit
	}

	for(int i=0; i<num_customer; i++)
	{
		//pthread_join(cus[i], NULL);
		cus[i].join();
		//cout<<"////pthread_cus"<<endl;
	}

	//	cout<<"////pthread_cus_exit"<<endl;
}

int main()
{

	mean = MEAN;
	timeRange = TIME_RANGE;
	num_customer = NUM_CUSTOMER;

	/*
    sem_init(&customers, 0, 0); // at first, no customer
    sem_init(&barbers, 0, NUM_BARBERS);
    sem_init(&Mutex, 0, 1);
    sem_init(&ioMutex, 0, 1);
    sem_init(&cusMutex, 0, 1);
    sem_init(&barMutex, 0, 1);
*/

	for(int i=0; i<NUM_CHAIRS; i++)  // fill the number of tn of all waiting chair
		waitingChairs[i].seqNumber = i;

	//pthread_t bar[NUM_BARBERS];
	thread bar[NUM_BARBERS];
	int barberID[NUM_BARBERS];

	int *cusArray = possionDistribution(mean, timeRange, num_customer); //Use p_s create

	for(int i=0; i<NUM_BARBERS; i++)
	{
		barberID[i] = i+1; // fill the barID
		//pthread_create(&bar[i], NULL, &barberThread, (void*)&barberID[i]);  // create all barber thread
        bar[i] = thread(&barberThread,(void*)&barberID[i]);
	}
	createCustomers(timeRange,num_customer,mean,cusArray);

	for(int i=0; i<NUM_BARBERS; i++)
	{
		cout<<"////pthread_bar"<<i<<endl;
		//pthread_join(bar[i], NULL);
		bar[i].join();

	}
	cout<<"////pthread_bar_exit"<<endl;
/*
    sem_destroy(&barbers);
    sem_destroy(&customers);
    sem_destroy(&Mutex);
    sem_destroy(&ioMutex);
    sem_destroy(&cusMutex);
    sem_destroy(&barMutex);
*/
	cout<<endl<<"All customers finish their haircuts!"<<endl;
	return 0;
}
