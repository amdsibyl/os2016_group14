// poisson_distribution
#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <random>

using namespace std;

// "p_d" represent the "poisson_distribution"

/*  This function need 3 parameter,and return the pointer to a int array(the length of array equal to second parameter)
    1. mean of p_d
    2. range of p_d (0~range)
    3. total number of people in this period
*/

struct customerData
{
    int cusID;
    bool hasFinishedCutting;
};
int nextID = 1;


int* possionDistribution(float, int, int);
void createCustomers();
void *customerThread(void *);

int main()
{
    createCustomers();
    return 0;
}

void createCustomers()
{
    float mean = 3.0;
    int num_customer = 10;  //how many customer will be create
    int timeRange = 10;    //1 period will have how many time unit
    pthread_t cus[num_customer];
    int cusTH = 0;      //this is n-th customer. (0 represent the first customer)
    struct customerData cusData[num_customer];
    
    int *cusArray = possionDistribution(mean, timeRange, num_customer); //Use p_s create 
    
    for(int i=0; i<timeRange; i++){
        for(int j=0; j<cusArray[i]; j++){

            cusData[cusTH].cusID = nextID;
            cusData[cusTH].hasFinishedCutting = false;

            cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< i << ")"<<endl;
            pthread_create(&cus[cusTH], NULL, customerThread, (void*)&cusData[cusTH]);

            cusTH ++;
            nextID ++;
            usleep(10);     // avoid create earlier but execute thread laterly
        }
        usleep(1000000);    // next time unit
    }

    for(int i=0; i<num_customer; i++){

        pthread_join(cus[i], NULL);
    }

}

int *possionDistribution(float mean, int range, int num_period){

    const int NUM_TIMES = num_period;

    std::default_random_engine generator;
    std::poisson_distribution<int> distribution(mean);

    int *frequenceArray = new int[range];
    int sum = 0;
    
    for(int i=0; i<range; i++)
		frequenceArray[i] = 0;
    
    for(int i=0; i<NUM_TIMES; i++){
        int number = distribution(generator);
        if(number < range)
            frequenceArray[number]++;
    }

    /* Output Result */
    for(int i=0; i<range; i++){
        cout << i << " : " << frequenceArray[i] <<endl;
        sum += frequenceArray[i];
    }
    cout << "Sum : " << sum << endl << endl;
    /* Output Result  */

    return frequenceArray;
}
void *customerThread(void *arg){
    struct customerData *data = (struct customerData*)arg;
    /*Customer's work*/
    
}
