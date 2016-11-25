#include <iostream>
#include <semaphore.h>
#include <pthread.h>

#define NUM_BARBERS 3
#define NUM_CHAIRS 5

using namespace std;

sem_t barber;
sem_t customer;
sem_t mutex;
int freeChair = NUM_CHAIRS;
int sleepingBarber = NUM_BARBERS;

int loopTime = 5;/*For test*/

void *barberThread(void* arg){
    
		/*
			if no customer -> go to sleep
			if have -> pick one
		*/

        /*Cut hair*/
    sem_post(&barber);


    pthread_exit(0);
}

void *customerThread(void* arg){
    if(sleepingBarber  != 0 ) {

	/* Wake up a barber */ 
	sem_post(&customer);
    	sem_wait(&barber);

    }
    else if(freeChair == 0)
        pthread_exit(0);
    else {
    	sem_wait(&mutex);
    	freeChair--;
    	sem_post(&mutex);

    	sem_post(&customer);
    	sem_wait(&barber);
    }
}

void createCustomer(){
    while(true){
        /* sleep random time */
        /* create customer   thread*/
    }
}
int main(){
    sem_init(&barber, 0, NUM_BARBERS);
    sem_init(&customer, 0, 0);
    sem_init(&mutex, 0, 1);

    pthread_t tid[NUM_BARBERS];
    for(int i=0 ; i<NUM_BARBERS; i++){
        pthread_create(&tid[i], NULL, barber, NULL);
    }
    for(int i=0 ; i<NUM_BARBERS; i++){
        pthread_join(tid[i]);
    }

}


