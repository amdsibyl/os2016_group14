#include<iostream>
#include<cstdio>
using namespace std;
/*
The barber shop has m barbers with m barber chairs, and n chairs (m < n) for waiting
customers, if any, to sit in. If there are no customers present, a barber sits down in a
barber chair and falls asleep. When a customer arrives, he has to wake up a sleeping
barber. If additional customers arrive while all barbers are cutting customers¡¦ hair, they
either sit down (if there are empty chairs) or leave the shop (if all chairs are full). The
thread synchronization problem is to program the barbers and the customers without
getting into race conditions.
*/
typedef int semaphore;

semaphore customers,barbers = m;
semaphore barberChairs = m,waitingChairs = n;
semaphore mutex;
int waitingCustomers;

void barber();
void costomer();
void up();
void down();

