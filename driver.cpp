#include <iostream>   // For input/output operations
#include <sys/time.h> // For gettimeofday function
#include <unistd.h>   // For usleep function
#include <cstdlib>    // For atoi function
#include <stdlib.h>   // For rand function
#include "Shop.h"     // Include the "Shop" class header file

using namespace std;

// Function prototypes
void *barber(void *);   // Prototype of the barber thread function
void *customer(void *); // Prototype of the customer thread function

// A set of parameters to be passed to each thread
class ThreadParam
{
public:
    ThreadParam(Shop *shop, int threadId, int serviceTime) : shop(shop), threadId(threadId), serviceTime(serviceTime){};
    Shop *shop;      // A pointer to the Shop object
    int threadId;    // A thread identifier
    int serviceTime; // Service time (in microseconds) for a barber, whereas 0 for a customer
};

int main(int argc, char *argv[])
{
    // Validate the arguments
    if (argc != 5)
    {
        cerr << "usage: sleepingBarber nBarbers nChairs nCustomers serviceTime" << endl;
        return -1;
    }
    int nBarbers = atoi(argv[1]);    // Number of barbers working in the barbershop
    int nChairs = atoi(argv[2]);     // Number of chairs available for customers to wait on
    int nCustomers = atoi(argv[3]);  // Number of customers who need a haircut service
    int serviceTime = atoi(argv[4]); // Service time for each barber (in microseconds)

    cout << "barbers: " << nBarbers << " chairs: " << nChairs << " customer: " << nCustomers << " serviceTime: " << serviceTime << endl;
    pthread_t barberThreads[nBarbers];
    pthread_t customerThreads[nCustomers];
    Shop shop(nBarbers, nChairs); // Instantiate a barbershop object

    // Create barber threads
    for (int i = 0; i < nBarbers; i++)
    {
        ThreadParam *barberParam = new ThreadParam(&shop, i, serviceTime);
        pthread_create(&barberThreads[i], NULL, barber, (void *)barberParam);
    }

    // Create customer threads
    for (int i = 0; i < nCustomers; i++)
    {
        usleep(rand() % 1000); // Sleep for a random interval before creating a customer thread
        ThreadParam *customerParam = new ThreadParam(&shop, i + 1, 0);
        pthread_create(&customerThreads[i], NULL, customer, (void *)customerParam);
    }

    // Wait until all customer threads are served
    for (int i = 0; i < nCustomers; i++)
        pthread_join(customerThreads[i], NULL);

    // Terminate all barber threads
    for (int i = 0; i < nBarbers; i++)
        pthread_cancel(barberThreads[i]);

    cout << "# customers who didn't receive a service = " << shop.numDropsOff << endl;

    return 0;
}

// The barber thread function
void *barber(void *arg)
{
    // Extract parameters
    ThreadParam &param = *(ThreadParam *)arg;
    Shop &shop = *(param.shop);
    int threadId = param.threadId;
    int serviceTime = param.serviceTime;
    delete &param;

    // Keep working until being terminated by the main thread
    while (true)
    {
        shop.helloCustomer(threadId); // Pick up a new customer
        usleep(serviceTime);          // Spend the service time
        shop.byeCustomer(threadId);   // Release the customer
    }
}

// The customer thread function
void *customer(void *arg)
{
    // Extract parameters
    ThreadParam &param = *(ThreadParam *)arg;
    Shop &shop = *(param.shop);
    int threadId = param.threadId;
    delete &param;

    int assignedBarber = -1;
    if ((assignedBarber = shop.visitShop(threadId)) != -1) // Check if assigned to a barber (i.e., assignedBarber != -1)
        shop.leaveShop(threadId, assignedBarber);          // Wait until my service is finished
}
