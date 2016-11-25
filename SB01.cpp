#include<iostream>
#include <cstdlib>
#include <queue>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <pthread.h>

#define m 6
#define n 6

using namespace std;
/*
The barber shop has m barbers with m barber chairs, and n chairs (m < n) for waiting
customers, if any, to sit in. If there are no customers present, a barber sits down in a
barber chair and falls asleep. When a customer arrives, he has to wake up a sleeping
barber. If additional customers arrive while all barbers are cutting customers' hair, they
either sit down (if there are empty chairs) or leave the shop (if all chairs are full). The
thread synchronization problem is to program the barbers and the customers without
getting into race conditions.
*/

/* a barber:a thread / a customer: a thread / waiting chairs:a semaphore / cut hair:a process */

/* barber room */
semaphore customers = 0;
semaphore barbers = m;
queue sleepingBarbers;

mutex mtx;//¿ïÅU«È

/* waiting room */
semaphore waitingChairs = n;
int waitingCustomers = 0;
/*I don't know we should use 'int' or 'semaphore' here*/

void barber();
void customer();

void barber(){
    /*if there is a customer?*/
    /*y:cut hair / n:sleep*/
}
void customer(){
    /*if there is an available barber?*/
    /*y:wake him up / n:check if there is a empty waiting seat->y:sit down / n:leave*/
}
