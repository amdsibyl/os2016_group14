// poisson_distribution
#include <iostream>
#include <random>

using namespace std;

// "p_d" represent the "poisson_distribution"

/*  This function need 3 parameter,and return the pointer to a int array(the length of array equal to second parameter)
    1. mean of p_d
    2. range of p_d (0~range)
    3. total number of people in this period
*/


int* possionDistribution(float, int, int);

int main()
{
    int num_cus_period = 100;
    int PD_range = 10; // PD for PossionDistribution
    float PD_mean = 3.0;

    int *Arr = possionDistribution(PD_mean, PD_range, num_cus_period);
    int *Arr2 = possionDistribution(50, 100, 50000);
    for(int i=0; i<PD_range; i++)
        cout << Arr[i] << endl;
}


int *possionDistribution(float mean, int range, int num_period){

    const int NUM_TIMES = num_period;

    std::default_random_engine generator;
    std::poisson_distribution<int> distribution(mean);

    int *frequenceArray = new int[range];
    int sum = 0;

    for(int i=0; i<NUM_TIMES; i++){
        int number = distribution(generator);
        if(number < range)
            frequenceArray[number]++;
    }
    
    /* Test  */
    for(int i=0; i<range; i++){
        cout << i << " : " << frequenceArray[i] <<endl;
        sum += frequenceArray[i];
    }
    cout << "Sum : " << sum << endl << endl;
    /* Test  */
    
    return frequenceArray;
}
