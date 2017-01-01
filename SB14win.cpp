#ifdef __APPLE__
#include <openGL/gl.h>
#include <openGL/glu.h>
#include <GLUT/glut.h>
#else
#include <windows.h>
//#include <GL/glew.h>
#include <GL/glut.h>
#endif

#include "RGBpixmap.h"

#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <random>
#include <iomanip>
#include <string.h>


#define NUM_BARBERS 3
#define NUM_CHAIRS 5

#define MEAN 3
#define TIME_RANGE 10
#define NUM_CUSTOMER 20

using namespace std;

//Set windows
int screenWidth = 800 , screenHeight = 600;

RGBApixmap bg,pic;
RGBApixmap barSleep,barBusy,chairPic,cusPic[20];

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

int cutting[NUM_BARBERS];
bool isBusy[NUM_BARBERS];
int seat[NUM_CHAIRS];
bool isSit[NUM_CHAIRS];
int comeCus[NUM_CUSTOMER];

void print(){
    /*
	cout<<"\n@@0 busy: "<<isBusy[0]<<endl;
	cout<<"\n@@1 busy: "<<isBusy[1]<<endl;
	cout<<"\n@@2 busy: "<<isBusy[2]<<endl;
    */
    system("CLS");
    cout<<"----------------------------------------------\n";
    cout<<"|                                            |\n";
    cout<<"| Barber                                     |\n";

    cout<<"|  ";
	for(int i=0;i<NUM_BARBERS;i++)
		cout<<" "<<setfill('0')<<setw(2)<<i+1<<"   ";
	for(int i=0;i<7-NUM_BARBERS;i++)
		cout<<"      ";
	cout<<"|\n";	//"01"

    cout<<"|  ";
	for(int i=0;i<NUM_BARBERS;i++)
		cout<<"----  ";
	for(int i=0;i<7-NUM_BARBERS;i++)
		cout<<"      ";
	cout<<"|\n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|  ";
	for(int i=0;i<NUM_BARBERS;i++){
		if(isBusy[i]&&cutting[i]!=-1){
			cout<<"|"<<setfill('0')<<setw(2)<<cutting[i]<<"|  ";
		}
		else
			cout<<"|--|  ";
	}
	for(int i=0;i<7-NUM_BARBERS;i++)
		cout<<"      ";
	cout<<"|\n";	//"| 1|  "

    cout<<"|  ";
	for(int i=0;i<NUM_BARBERS;i++)
		cout<<"----  ";
	for(int i=0;i<7-NUM_BARBERS;i++)
		cout<<"      ";
	cout<<"|\n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|                                            |\n";
    cout<<"|  Chair                                    / \n";

    cout<<"|  ";
	for(int i=0;i<NUM_CHAIRS;i++)
		cout<<"----  ";
	for(int i=0;i<6-NUM_CHAIRS;i++)
		cout<<"      ";
	cout<<"    /  \n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|  ";
	for(int i=0;i<NUM_CHAIRS;i++){
		if(isSit[i] && seat[i]!=-1){
			cout<<"|"<<setfill('0')<<setw(2)<<seat[i]<<"|  ";
		}
		else
			cout<<"|--|  ";
	}
	for(int i=0;i<6-NUM_CHAIRS;i++)
		cout<<"      ";
	cout<<"   / ";
	// coming customers
	for(int i=0;i<NUM_CUSTOMER;i++){
		if(comeCus[i]==true)
			cout<<i+1<<" ";
	}
	cout<<"\n";	//cout<<"|                                         /   \n";

    cout<<"|  ";
	for(int i=0;i<NUM_CHAIRS;i++)
		cout<<"----  ";
	for(int i=0;i<6-NUM_CHAIRS;i++)
		cout<<"      ";
	cout<<"  /    \n";	//cout<<"|  ----  ----  ----  ----  ----  ----  ----  |\n";

    cout<<"|                                            |\n";
    cout<<"----------------------------------------------\n";
}

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
        if(waitingChairs[i].data != nullptr){
			isSit[i] = true;
            cout << waitingChairs[i].data->cusID << " ";
		}
        else{
			isSit[i] = false;
		   	cout << "0 ";
		}
    }
    cout << endl;
	print();
    sem_post(&ioMutex); // Release waiting
}

void cutHair(int barberID, Chair wChair)
{
	isBusy[barberID-1] = true;
    glutPostRedisplay();

    sem_wait(&cusMutex);
    ++totalServedCustomers;
    sem_post(&barMutex);

    sem_wait(&ioMutex); // Acquire access to waiting
    cout<<"total:"<<totalServedCustomers<<"/"<<realNum_customer<<endl;
    sem_post(&ioMutex); // Release waiting

    sem_post(&cusMutex);

    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "(B) Barber " << barberID <<" is cutting Customer No." << wChair.data->cusID << "'s hair !"<<endl;
    //cout << "(At chair No." << wChair.seqNumber << ")" << endl;
	cutting[barberID-1] = wChair.data->cusID;
	print();
    sem_post(&ioMutex); // Release waiting

    waitingChairs[wChair.seqNumber].data = nullptr;
	isSit[wChair.seqNumber] = false;

    for(long long i=0; i<200000000; i++); //Cut hair time
	isBusy[barberID-1] = false;
    glutPostRedisplay();

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
        sem_wait(&cusMutex);
        //		sem_wait(&ioMutex);
        //		cout<<"\n----- "<<totalServedCustomers<<" / "<<cus_perTime[currentTime]<<" -----\n";
        //		sem_post(&ioMutex);
        if(totalServedCustomers >= realNum_customer){
            sem_post(&cusMutex);
            //			sem_wait(&ioMutex);
            //			cout<<"\n@@ Barber "<<*pID<<" e04!!\n\n";
            //			sem_post(&ioMutex);
            break;
        }
        /*
         if(totalServedCustomers >= cus_perTime[currentTime] && totalServedCustomers != realNum_customer){
         sem_post(&cusMutex);
         continue;
         }
         */
        sem_post(&cusMutex);

        sem_wait(&ioMutex);
        cout<<"\nBarber "<<*pID<<" is free!!\n\n";
        sem_post(&ioMutex);

    sem_wait(&ioMutex); // Acquire access to waiting
    print();
    sem_post(&ioMutex); // Release waiting

        sem_wait(&barMutex);
        //				sem_wait(&ioMutex);
        //				cout<<"\n----- "<<totalServedCustomers<<" / "<<cus_perTime[currentTime]<<" -----\n";
        //				sem_post(&ioMutex);
        if(totalServedCustomers < realNum_customer){
            //			sem_wait(&ioMutex);
            //			cout<<"\n@1 Barber "<<*pID<<" cus\n\n";
            //			sem_post(&ioMutex);
            sem_wait(&customers); // Try to acquire a customer.

            //Go to sleep if no customers
            //			sem_wait(&ioMutex);
            //			cout<<"\n@2 Barber "<<*pID<<" mtx\n\n";
            //			sem_post(&ioMutex);
            sem_wait(&Mutex); // Acquire access to waiting

            //When a barber is waken -> wants to modify # of available chairs
            //			sem_wait(&ioMutex);
            //			cout<<"\n@2 Barber "<<*pID<<" barbers\n\n";
            //			sem_post(&ioMutex);
            sem_post(&barbers);  // The barber is now ready to cut hair


            int nowCut = nextCut;
            nextCut = (nextCut+1) % NUM_CHAIRS;
            availableChairs++;

            sem_post(&Mutex); // Release waiting
            cutHair(*pID, waitingChairs[nowCut]); //pick the customer which counter point
        }
        else{
            sem_wait(&ioMutex);
            cout<<"\n@2 Barber "<<*pID<<" arrrrr\n\n";
            sem_post(&ioMutex);
            sem_post(&barMutex);
        }
    }
    //pthread_exit(0);
}


void waitForHairCut(struct customerData *a)
{
    while(a->hasFinishedCutting == false);
}

void *customerThread(void* arg)
{
    struct customerData *data = (struct customerData*)arg;
    sem_wait(&Mutex);
    if( availableChairs == 0 )
    {
        sem_wait(&ioMutex); // Acquire access to waiting
        cout << "There is no available chair. Customer No." << data->cusID << " is leaving!" << endl;
		comeCus[data->cusID-1] = false;
        sem_post(&ioMutex); // Release waiting

        sem_wait(&cusMutex);
        --realNum_customer;
        --cus_perTime[currentTime];
        sem_post(&cusMutex);

        sem_post(&Mutex);
        pthread_exit(0);
    }
    sem_wait(&ioMutex); // Acquire access to waiting
    cout << "Customer No." << data->cusID << " is sitting on chair " << nextSit << "." << endl;
	comeCus[data->cusID-1] = false;
	seat[nextSit] = data->cusID;
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
    //pthread_exit(0);
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
        cus_perTime[i] = realNum_customer;
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
        sem_wait(&ioMutex); // Acquire access to waiting
        cout<<endl<<"*****TIME:"<<currentTime<<endl;
        sem_post(&ioMutex); // Release waiting
        for(int j=0; j<cusArray[currentTime]; j++)
        {
            cusData[cusTH].cusID = nextID;
            cusData[cusTH].hasFinishedCutting = false;

            sem_wait(&ioMutex); // Acquire access to waiting
            cout <<endl<< "Create Customer No."<< cusData[cusTH].cusID <<".\t(now Time :"<< currentTime << ")"<<endl;
			comeCus[cusData[cusTH].cusID-1] = true;
			print();
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


/***************************** OpenGL *********************************/




static void CheckError(int line)
{
    GLenum err = glGetError();
    if (err) {
        printf("GL Error %s (0x%x) at line %d\n",
               gluErrorString(err), (int) err, line);
    }
}

void reshape(int w, int h)
{
    /* Save the new width and height */
    screenWidth  = w;
    screenHeight = h;
    
    /* Reset the viewport... */
    glViewport(0, 0, screenWidth, screenHeight);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glOrtho(0.0, (GLfloat)screenWidth, 0.0, (GLfloat)screenHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    //draw background
    glRasterPos2i(50, 50);
    bg.blend();
    
    /* chairs */
    chairPic.blendTexRotate(30, 200,1,1);
    chairPic.blendTexRotate(180, 200,1,1);
    chairPic.blendTexRotate(330, 200,1,1);
    chairPic.blendTexRotate(480, 200,1,1);
    chairPic.blendTexRotate(630, 200,1,1);
    
    /* barber 1 */
    char fpsmss[30];
    sprintf(fpsmss,"%d",isBusy[0]);
    glColor3f(0.0, 0.5, 0.3);  //set font color
    glRasterPos2i(screenWidth-100, 550);    //set font start position
    for(int i=0; i<strlen(fpsmss); i++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, fpsmss[i]);
    }
    
    if(isBusy[0])
        barBusy.blendTexRotate(50, 450,1,1);
    else
        barSleep.blendTexRotate(50, 450,1,1);
    //cusPic[0].blendTexRotate(50, 450,0.95,0.95);
    
    /* barber 2 */
    barSleep.blendTexRotate(200, 450,1,1);
    
    /* barber 3 */
    barSleep.blendTexRotate(350, 450,1,1);
    
    CheckError(__LINE__);
    glutSwapBuffers();
}

void keys(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 'Q':
        case 'q':
            exit(0);
            break;
            
        case ' ':
            sem_init(&customers, 0, 0); // at first, no customer
            sem_init(&barbers, 0, NUM_BARBERS);
            sem_init(&Mutex, 0, 1);
            sem_init(&ioMutex, 0, 1);
            sem_init(&cusMutex, 0, 1);
            sem_init(&barMutex, 0, 1);
            
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
            
            for(int i=0;i<NUM_BARBERS;i++){
                cutting[i] = -1;
                isBusy[i] = false;
            }
            for(int i=0;i<NUM_CUSTOMER;i++){
                comeCus[i] = false;
            }
            
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
            break;
            
    } //switch(key)
    
    glutPostRedisplay();
}


void init( void )
{
    glClearColor( 1.0, 1.0, 1.0, 0.0 );
   	glutDisplayFunc(display);
    glutKeyboardFunc(keys);
    glutReshapeFunc(reshape);
    
    glShadeModel(GL_SMOOTH);
}


int main( int argc, char** argv )
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(50, 30);
    glutCreateWindow("Barber Sleeping Problem");
    
    init();
    
    
    barSleep.readBMPFile("/Users/HSUAN/os2016_group14/pic/barber_sleep.bmp");
    barBusy.readBMPFile("/Users/HSUAN/os2016_group14/pic/barber_busy.BMP");
    chairPic.readBMPFile("pic/chair.BMP");
    cusPic[0].readBMPFile("pic/cus_1.BMP");
    cusPic[1].readBMPFile("pic/cus_2.BMP");
    cusPic[2].readBMPFile("pic/cus_3.BMP");
    cusPic[3].readBMPFile("pic/cus_4.BMP");
    cusPic[4].readBMPFile("pic/cus_5.BMP");
    cusPic[5].readBMPFile("pic/cus_6.BMP");
    cusPic[6].readBMPFile("pic/cus_7.BMP");
    cusPic[7].readBMPFile("pic/cus_8.BMP");
    cusPic[8].readBMPFile("pic/cus_9.BMP");
    cusPic[9].readBMPFile("pic/cus_10.BMP");
    cusPic[10].readBMPFile("pic/cus_11.BMP");
    cusPic[11].readBMPFile("pic/cus_12.BMP");
    cusPic[12].readBMPFile("pic/cus_13.BMP");
    cusPic[13].readBMPFile("pic/cus_14.BMP");
    cusPic[14].readBMPFile("pic/cus_15.BMP");
    cusPic[15].readBMPFile("pic/cus_16.BMP");
    cusPic[16].readBMPFile("pic/cus_17.BMP");
    cusPic[17].readBMPFile("pic/cus_18.BMP");
    cusPic[18].readBMPFile("pic/cus_19.BMP");
    cusPic[19].readBMPFile("pic/cus_20.BMP");
    
    barSleep.setChromaKey(255, 255, 255);
    barBusy.setChromaKey(255, 255, 255);
    chairPic.setChromaKey(255, 255, 255);
    for (int i=0; i<20; i++)
        cusPic[i].setChromaKey(255, 255, 255);
    
    
    glutMainLoop();
    
    
    return 0;
}
