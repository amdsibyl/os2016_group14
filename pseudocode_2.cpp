#include <semaphore.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>

#define NUM_CHAIRS 5
#define NUM_BARBERS 3
using namespace std;

int freeChair = NUM_CHAIRS;
int sleepingBarber = 0;

sem_t customer;
sem_t barber;
sem_t mutex;

int nextID = 1;  /* ID for customer */


typedef struct chair{ 
    int cusID;
    int th;
}Chair;     /*   A struct of chair which contain tn(this chair is number tn of all chair) and 
                 cusID (record the ID for the customer who currently has this chair, if nobody has it, then cusID = 0
            */

Chair waitingChair[NUM_CHAIRS];
int nextCut = 0;    /*  Point to the chair which next servered customer sit on  */
int nextSit = 0;    /*  Point to the chair which will be sit when next customer come */

void showChair(){   
    for(int i=0;i<NUM_CHAIRS;i++)
        cout << waitingChair[i].cusID << " ";
    cout << endl;
}
void cutting(int barberID, Chair wChair){
    nextCut = (nextCut+1) % NUM_CHAIRS;
    waitingChair[wChair.th].cusID = 0;
    freeChair ++;
    cout << "Barber " << barberID <<" is cutting Customer " << wChair.cusID << " hair !(from chair " << wChair.th << ")" << endl;
    for(long i=0; i<100000000; i++); /* Test */
    cout << "Barber " << barberID <<" just finish cutting Customer " << wChair.cusID << "!" <<endl<<endl;;

}
void *barberThread(void* arg){
    int *pID = (int*)arg;
    cout << "I'm Barber " << *pID << endl;
    while(true){
        sem_wait(&customer); // if customer = 0, then sleep
        /* Has customer in waiting chair*/
        cutting(*pID, waitingChair[nextCut]); //pick the customer which counter point
    }
}
void *customerThread(void *arg){
    int *pID = (int*)arg;
    if(freeChair ==0 )  { 
        cout << "(" << *pID << ") No chair !I'm leaving!" << endl;
        pthread_exit(0);
    }
    sem_wait(&mutex);
        cout << "Customer " << *pID << " sit on chair : " << nextSit << "\t";
        waitingChair[nextSit].cusID = *pID;
        nextSit = (nextSit+1) % NUM_CHAIRS;
        freeChair --;
        showChair();
    sem_post(&mutex);
    sem_post(&customer);
}
void createCustomer(){
    int loopTime = 10;  /* Test */
    int cusID[loopTime];
    pthread_t cus[loopTime];
    for(int i=0; i<loopTime; i++){
        cusID[i] = i+1;
        pthread_create(&cus[i], NULL, customerThread, (void*)&cusID[i]);
        cout << "Create Customer "<< cusID[i] << endl;

        usleep(100000);  /* Test */
    }

}

int main(){
    sem_init(&customer, 0, 0);        // at first, no customer
    sem_init(&barber, 0, NUM_BARBERS);
    sem_init(&mutex, 0, 1);
    
    pthread_t bar[NUM_BARBERS];
    int barID[NUM_BARBERS];     

    for(int i=0; i<NUM_CHAIRS; i++)  // fill the number of tn of all waiting chair
        waitingChair[i].th = i; 
    for(int i=0; i<NUM_BARBERS; i++){ 
        barID[i] = i+1;              // fill the barID
        pthread_create(&bar[i], NULL, barberThread, (void*)&barID[i]);  // create all barber thread
    }

    createCustomer();

    for(int i=0; i<NUM_BARBERS; i++){
        pthread_join(bar[i], NULL);
    }

    return 0;
}
