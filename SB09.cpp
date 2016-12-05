#include <iostream>
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
sem_t barbers;/*Number of barbers waiting for customers*/
sem_t customers;/*Number of customers waiting for service*/
sem_t mutex;/*Mutex used for mutual exclusion*/
sem_t ioMutex;/*Mutex used for input and output*/

/*for poisson distribution*/
int timeRange; //1 period will have how many time unit
int num_customer; //how many customer will be create

struct customerData
{
    int cusID;
    bool hasFinishedCutting;
};

typedef struct chair
{
    struct customerData *data;
    //int customerID;/*Number of the customer who sits on the chair*/
    /*If nobody sits on it, then customerID = 0*/
    int seqNumber;/*The chair's sequence number of all chairs*/
} Chair;

/*Chairs for waiting area*/
Chair waitingChairs[NUM_CHAIRS];

/*Number of available waiting chairs*/
int availableChairs = NUM_CHAIRS;

int nextID = 1;  /* ID for customer */

int nextCut = 0;    /*  Point to the chair which next served customer sits on  */
int nextSit = 0;    /*  Point to the chair which will be sat when next customer comes */

int totalServedCustomers = 0;    /*number of customers that have been served by barbers*/

void showWhoSitOnChair()
{
    sem_wait(&ioMutex); // Acquire access to waiting
    cout<<"Waiting Chairs:";
    for(int i=0; i<NUM_CHAIRS; i++){
        if(waitingChairs[i].data != nullptr)
            cout << waitingChairs[i].data->cusID << " ";
        else cout << "0 ";
    }
    cout << endl;
    sem_post(&ioMutex); // Release waiting
}

void cutHair(int barberID, Chair wChair)
{
    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "(B) Barber " << barberID <<" is cutting Customer No." << wChair.data->cusID << "'s hair !"<<endl;
    //cout << "(At chair No." << wChair.seqNumber << ")" << endl;
    sem_post(&ioMutex); // Release waiting

    waitingChairs[wChair.seqNumber].data = nullptr;

    for(long long i=0; i<200000000;i++); //Cut hair time

    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "(B)Barber " << barberID <<" just finished cutting Customer No." << wChair.data->cusID << "'s hair !" <<endl<<endl;;
    sem_post(&ioMutex); // Release waiting

	wChair.data->hasFinishedCutting = true;
	totalServedCustomers ++;
}

void *barberThread(void* arg)
{
    int *pID = (int*)arg;

    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "This is Barber No." << *pID << endl;
    sem_post(&ioMutex); // Release waiting

    while(totalServedCustomers < num_customer)
    {
        /*
        sem_wait(&ioMutex); // Acquire access to waiting
        cout<<totalServedCustomers<<" , "<<num_customer<<endl;
        sem_post(&ioMutex); // Release waiting
        */

        sem_wait(&customers); // Try to acquire a customer.
        //Go to sleep if no customers

        sem_wait(&mutex); // Acquire access to waiting
        //When a barber is waken -> wants to modify # of available chairs

        sem_post(&barbers);  // The barber is now ready to cut hair

        int nowCut = nextCut;
        nextCut = (nextCut+1) % NUM_CHAIRS;
        availableChairs++;

        sem_post(&mutex); // Release waiting
        //don't need to lock on the chair anymore

        cutHair(*pID, waitingChairs[nowCut]); //pick the customer which counter point

    }
}

void waitHairCut(struct customerData *a)
{
    while(a->hasFinishedCutting == false);
}

void *customerThread(void* arg)
{
    struct customerData *data = (struct customerData*)arg;
    //int *pID = (int*)arg;
    sem_wait(&mutex); // Acquire access to waiting
    //execute a DOWN on mutex before entering critical section

    if( availableChairs == 0 )
    {
        sem_wait(&ioMutex); // Acquire access to waiting
        cout << "There is no available chair. Customer No." << data->cusID << " is leaving!" << endl;
        //cout << "There is no available chair. Customer No." << *pID << " is leaving!" << endl;
        sem_post(&ioMutex); // Release waiting

        sem_post(&mutex);
        pthread_exit(0);
    }
    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "Customer No." << data->cusID << " is sitting on chair " << nextSit << "." << endl;
    //cout << "Customer No." << *pID << " is sitting on chair " << nextSit << "." << endl;
    sem_post(&ioMutex); // Release waiting

    waitingChairs[nextSit].data = data;
    //waitingChairs[nextSit].customerID = *pID;
    nextSit = (nextSit+1) % NUM_CHAIRS;
    availableChairs--;
    showWhoSitOnChair();

    sem_post(&customers); // Wake up a barber (if needed)
    sem_post(&mutex); // Release waiting
    //execute an UP on mutex when leaving critical section

    sem_wait(&barbers); // Go to sleep if number of available barbers is 0
    waitHairCut(data);
    //getHairCut(&data);

    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "(C)Customer No." << data->cusID <<" just finished his haircut!"<<endl;
    //cout << "#Customer No." << *pID <<" just finished his haircut!"<<endl;;
    sem_post(&ioMutex); // Release waiting

}

int *possionDistribution(float mean, int range, int num_period){

    const int NUM_TIMES = num_period;

    default_random_engine generator;
    poisson_distribution<int> distribution(mean);

    int *frequenceArray = new int[range];
    //int sum = 0;

    for(int i=0; i<range; i++)
		frequenceArray[i] = 0;

    for(int i=0; i<NUM_TIMES; i++){
        int number = distribution(generator);
        if(number < range)
            frequenceArray[number]++;
    }

    /* Output Result */
	/*
	sem_wait(&ioMutex);
    for(int i=0; i<range; i++){
        cout << i << " : " << frequenceArray[i] <<endl;
        sum += frequenceArray[i];
    }
    cout << "Sum : " << sum << endl << endl;
	sem_post(&ioMutex);
	*/
    /* Output Result  */

    return frequenceArray;
}

void createCustomers(int timeRange,int num_customer)
{
    float mean = 3.0;
    pthread_t cus[num_customer];
    int cusTH = 0;      //this is n-th customer. (0 represent the first customer)
    struct customerData cusData[num_customer];
    //int cusID[num_customer];

    int *cusArray = possionDistribution(mean, timeRange, num_customer); //Use p_s create

    for(int i=0; i<timeRange; i++){
        sem_wait(&ioMutex); // Acquire access to waiting
        cout<< "*****TIME:" <<i <<endl;
        sem_post(&ioMutex); // Release waiting
        for(int j=0; j<cusArray[i]; j++){
/*
            cusID[cusTH] = nextID;
            sem_wait(&ioMutex); // Acquire access to waiting
            cout <<endl<< "Create Customer No."<< cusID[cusTH] <<"\t(now Time :"<< i << ")"<<endl;
            sem_post(&ioMutex); // Release waiting
            pthread_create(&cus[cusTH], NULL, customerThread, (void*)&cusID[cusTH]);
*/
            cusData[cusTH].cusID = nextID;
            cusData[cusTH].hasFinishedCutting = false;

            sem_wait(&ioMutex); // Acquire access to waiting
            cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< i << ")"<<endl;
            sem_post(&ioMutex); // Release waiting

            pthread_create(&cus[cusTH], NULL, customerThread, (void*)&cusData[cusTH]);

            cusTH ++;
            nextID ++;
            usleep(10);     // avoid create earlier but execute thread laterly
        }
        usleep(1000000);    // next time unit
    }

    for(int i=0; i<num_customer; i++){
        pthread_join(cus[i], NULL);
        //cout<<"////pthread_cus"<<endl;
    }
    //cout<<"////pthread_cus_exit"<<endl;
}

int main()
{
    cout<<"Enter the time range (sec) that you want to test:";
    cin>>timeRange;
    cout<<"Enter number of customers that you want to create:";
    cin>>num_customer;

    sem_init(&customers, 0, 0); // at first, no customer
    sem_init(&barbers, 0, NUM_BARBERS);
    sem_init(&mutex, 0, 1);
    sem_init(&ioMutex, 0, 1);

    for(int i=0; i<NUM_CHAIRS; i++)  // fill the number of tn of all waiting chair
        waitingChairs[i].seqNumber = i;

    pthread_t bar[NUM_BARBERS];
    int barberID[NUM_BARBERS];

    for(int i=0; i<NUM_BARBERS; i++)
    {
        barberID[i] = i+1; // fill the barID
        pthread_create(&bar[i], NULL, barberThread, (void*)&barberID[i]);  // create all barber thread
    }

    createCustomers(timeRange,num_customer);

    for(int i=0; i<NUM_BARBERS; i++){
        pthread_join(bar[i], NULL);
        cout<<"////pthread_bar"<<endl;
    }
    //cout<<"////pthread_bar_exit"<<endl;

    return 0;
}
