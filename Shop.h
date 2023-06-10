#ifndef __SHOP_H__
#define __SHOP_H__

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <queue>
#include <deque>
#include <unordered_map>
#include <string>

using namespace std;

#define DEFAULT_CHAIRS 3
#define DEFAULT_BARBERS 1
#define errExitEN(en, msg)  \
    do                      \
    {                       \
        errno = en;         \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

class Shop
{
public:
    // Constructor with specified number of barbers and chairs
    Shop(int numBarbers, int numChairs);

    // Default constructor with default number of barbers and chairs
    Shop();

    // Destructor
    ~Shop();

    // Simulates a customer visiting the shop and returns the assigned barber ID
    int visitShop(int customerId);

    // Notifies the shop that a customer is leaving
    void leaveShop(int customerId, int barberId);

    // Greets a customer by the barber with the given ID
    void helloCustomer(int barberId);

    // Bids farewell to a customer by the barber with the given ID
    void byeCustomer(int barberId);

    int numDropsOff; // Counter for customers who leave without being served

private:
    // Initializes the shop and its components
    void initialize();

    // Converts an integer to a string
    inline string intToString(int i);

    // Prints a message with the person's ID
    inline void print(int personId, string message);

    // Checks if all chairs in the waiting area are occupied
    bool isAllChairsFull();

    // Checks if all barbers are currently serving customers
    bool isAllBarbersBusy();

    int numBarbers;         // Total number of barbers
    int numChairs;          // Total number of chairs in the waiting area
    int numAvailableChairs; // Number of currently available chairs in the waiting area

    deque<int> freeBarbers;     // Queue to hold the IDs of barbers who are currently free
    vector<int> chairOccupancy; // Vector to track the occupation status of chairs for customers or barbers

    // Separate critical sections for barbers and customers
    pthread_mutex_t barberMutex;   // Mutex lock for barber-related operations
    pthread_mutex_t customerMutex; // Mutex lock for customer-related operations

    pthread_cond_t customerWaitingCond;                                  // Condition variable for customers waiting in the waiting area
    pthread_cond_t *barberWaitingConds;                                  // Array of condition variables for each barber indicating their availability
    unordered_map<int, pthread_cond_t *> customerWaitingForServiceConds; // Map to store condition variables for each customer waiting for service
};
#endif
