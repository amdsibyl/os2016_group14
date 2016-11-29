// poisson_distribution
#include <iostream>
#include <random>

using namespace std;

int* possionDistribution(int, int, int);

int main()
{                                               /* "p_d" for "poisson_distribution"*/
    int *Arr = possionDistribution(5, 10, 100); /*(mean of p_d, range of p_d, num of frequence of a period)*/
    int *Arr2 = possionDistribution(50, 100, 50000);
    for(int i=0; i<10; i++)
        cout << Arr[i] << endl;
}


int *possionDistribution(int mean, int range, int num_period){

    const int NUM_TIMES = 10000;


    std::default_random_engine generator;
    std::poisson_distribution<int> distribution(mean);

    int frequence[range] = {0};

    for(int i=0; i<NUM_TIMES; i++){
        int number = distribution(generator);
        if(number < range)frequence[number]++;
    }
    int sum = 0;
    for(int i=0; i<range; i++){
        frequence[i] = frequence[i] * num_period / NUM_TIMES;
        cout << i << " : " << frequence[i] <<endl;
        sum += frequence[i];
    }
    cout << "Sum : " << sum << endl << endl;
    int *frequArr = new int[range];
    for(int i=0; i<range; i++)
        frequArr[i] =  frequence[i];
    return frequArr;
}
