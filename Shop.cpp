#include "Shop.h"
#include <sys/time.h>

#define WAIT_TIME_SECONDS 1
Shop::Shop(int numBarbers, int numChairs)
{
    this->numBarbers = numBarbers;
    this->numChairs = numChairs;
    this->numDropsOff = 0;
    this->numAvailableChairs = numChairs;
    this->chairOccupancy.resize(numBarbers, -1); // initized as no customers
    initialize();
}

Shop::Shop()
{
    this->numBarbers = DEFAULT_BARBERS;
    this->numChairs = DEFAULT_CHAIRS;
    this->numDropsOff = 0;
    this->numAvailableChairs = numChairs;
    this->chairOccupancy.resize(numBarbers, -1);
    initialize();
}

Shop::~Shop()
{
    pthread_cond_destroy(&this->customerWaitingCond);
    for (int i = 0; i < this->numBarbers; ++i)
    {
        pthread_cond_destroy(&this->barberWaitingConds[i]);
    }

    pthread_mutex_destroy(&barberMutex);
    pthread_mutex_destroy(&customerMutex);
    delete[] barberWaitingConds;

    for (auto it = this->customerWaitingForServiceConds.begin(); it != this->customerWaitingForServiceConds.end(); ++it)
    {
        pthread_cond_destroy(it->second); // destroy all prhread conditions
    }
}

void Shop::initialize()
{
    /* initialize a condition variable to its default value */
    int ret = pthread_cond_init(&this->customerWaitingCond, NULL);
    if (ret != 0)
    {
        errExitEN(ret, "pthread_cond_init");
    }

    this->barberWaitingConds = new pthread_cond_t[this->numBarbers];
    for (int i = 0; i < this->numBarbers; ++i)
    {
        ret = pthread_cond_init(&this->barberWaitingConds[i], NULL);
        if (ret != 0)
        {
            errExitEN(ret, "pthread_cond_init");
        }
    }

    ret = pthread_mutex_init(&this->barberMutex, NULL);
    if (ret != 0)
    {
        errExitEN(ret, "pthread_mutex_init");
    }

    ret = pthread_mutex_init(&this->customerMutex, NULL);
    if (ret != 0)
    {
        errExitEN(ret, "pthread_mutex_init");
    }
}

// is called by a customer thread. return a avaiable barberID
int Shop::visitShop(int id)
{
    // center the customer critical section
    int rc = pthread_mutex_lock(&customerMutex);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_lock");
    }

    // if all chair are full
    if (this->numAvailableChairs == 0)
    {
        // Print “id leaves the shop because of no available waiting chairs”.
        this->print(id, " leaves the shop because of no available waiting chairs");
        // Increment numDropsOff.
        this->numDropsOff += 1;

        // Leave the critical section.
        rc = pthread_mutex_unlock(&customerMutex);
        if (rc != 0)
        {
            errExitEN(rc, "visitShop:pthread_mutex_unlock");
        }

        return -1;
    }

    // Take a waiting char (or Push the customer in a waiting queue).
    this->numAvailableChairs = numAvailableChairs - 1;

    // Print “id takes a waiting chair. # waiting seats available = …”.
    this->print(id, " takes a waiting chair. # waiting seats available = " + intToString(this->numAvailableChairs));

    while (this->freeBarbers.empty()) // when no barbers avaiable
    {
        // Wait for a barber to wake me up.
        rc = pthread_cond_wait(&this->customerWaitingCond, &this->customerMutex);
        if (rc != 0)
        {
            errExitEN(rc, "visitShop:pthread_cond_wait");
        }
    }

    // when there is avaiable babers, enter the barber critical section
    rc = pthread_mutex_lock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_lock");
    }

    if (this->freeBarbers.empty())
    {
        cout << "error "
             << "this->freeBarbers is empty" << endl;
        exit(EXIT_FAILURE);
    }

    // get barber id
    int barberId = this->freeBarbers.front();
    this->freeBarbers.pop_front(); // remove the barberid from the list

    // leave the barber critical section
    rc = pthread_mutex_unlock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_unlock");
    }

    numAvailableChairs = this->numAvailableChairs + 1;

    rc = pthread_mutex_unlock(&customerMutex);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_unlock");
    }

    return barberId;
}

// is called by a customer thread.
void Shop::leaveShop(int customerId, int barberId)
{
    // enter the barber critical section
    int rc = pthread_mutex_lock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_mutex_lock");
    }

    pthread_cond_t *waitforservicing = new pthread_cond_t();
    rc = pthread_cond_init(waitforservicing, NULL);
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_cond_init");
    }

    this->customerWaitingForServiceConds.insert(make_pair(customerId, waitforservicing)); // save customer & wait condition into the hashmap
    this->chairOccupancy[barberId] = customerId;                                          // the customer sits on the chair served by the barberID
    this->print(customerId, " moves to a service chair[" + intToString(barberId) + "], # waiting seats available = " + intToString(this->numAvailableChairs));
    rc = pthread_cond_signal(&this->barberWaitingConds[barberId]); // the customer thread signals the barberID provide service
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_cond_signal");
    }

    this->print(customerId, " wait for barber[" + intToString(barberId) + "] to be done with hair-cut.");

    // while the barberID is curtting my hair, wait.
    while (this->chairOccupancy[barberId] == customerId)
    {
        struct timespec ts;
        struct timeval tp;
        rc = gettimeofday(&tp, NULL);
        if (rc != 0)
        {
            errExitEN(rc, "leaveShop:gettimeofday");
        }
        /* Convert from timeval to timespec */
        ts.tv_sec = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;

        // the customer thread waits for a certain time and then check wether service done. use timedwait to avoid deadlock.
        rc = pthread_cond_timedwait(this->customerWaitingForServiceConds[customerId], &this->barberMutex, &ts);
        if (rc != 0 && rc != ETIMEDOUT)
        {
            errExitEN(rc, "leaveShop:pthread_cond_timedwait");
        }
    }

    this->print(customerId, " says good-by to barber[" + intToString(barberId) + "]");
    // leave the critical section
    rc = pthread_mutex_unlock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_mutex_unlock");
    }
};

// is called by a barber thread.
// para: id is the barber's id
void Shop::helloCustomer(int id)
{
    // enter barber critical section
    int rc = pthread_mutex_lock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "helloCustomer:pthread_mutex_lock");
    }

    struct timespec ts;
    struct timeval tp;
    while (this->chairOccupancy[id] == -1) // the barber thread has no customer.
    {
        rc = gettimeofday(&tp, NULL);
        if (rc != 0)
        {
            errExitEN(rc, "helloCustomer:gettimeofday");
        }
        /* Convert from timeval to timespec */
        ts.tv_sec = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;

        bool f = false;
        for (deque<int>::iterator it = freeBarbers.begin(); it != freeBarbers.end(); ++it) // check whether the barber thread free
        {
            if (*it == id)
            {
                f = true;
                break;
            }
        }

        if (!f)
        {
            this->freeBarbers.push_back(id);
        }

        if (this->numAvailableChairs == 0)
        {
            this->print(-id, " sleeps because of no customers.");
        }

        pthread_cond_signal(&this->customerWaitingCond);
        if (rc != 0)
        {
            errExitEN(rc, "helloCustomer:pthread_cond_signal");
        }
        // the barber wait for a customer to give a signal within a time range. then go back to check any customer sit on his chair.
        rc = pthread_cond_timedwait(&this->barberWaitingConds[id], &this->barberMutex, &ts);
        if (rc != 0 && rc != ETIMEDOUT)
        {
            errExitEN(rc, "helloCustomer:pthread_cond_timedwait");
        }
    }

    this->print(-id, " starts a hair-cut service for customer[" + intToString(this->chairOccupancy[id]) + "].");
    // get a customer, release critical section to the other barbers.
    rc = pthread_mutex_unlock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "helloCustomer:pthread_mutex_unlock");
    }
}

// is called by a barber thread.
void Shop::byeCustomer(int id)
{
    // start a critical section
    int rc = pthread_mutex_lock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_mutex_lock");
    }

    this->print(-id, " says he's done with a hair-cut service for customer[" + intToString(this->chairOccupancy[id]) + "].");

    // wakes up the customer who seat on his chair
    rc = pthread_cond_signal(this->customerWaitingForServiceConds[this->chairOccupancy[id]]);
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_cond_signal");
    }

    this->freeBarbers.push_back(id);                      // go back to freebarber list
    this->chairOccupancy[id] = -1;                        // remove the customer form his chair
    rc = pthread_cond_signal(&this->customerWaitingCond); // signal the cutomer service is done.
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_cond_signal");
    }

    this->print(-id, " calls in another customer.");
    rc = pthread_mutex_unlock(&barberMutex);
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_mutex_unlock");
    }
}

string inline Shop::intToString(int i)
{
    stringstream out;
    out << i;
    return out.str();
}

void inline Shop::print(int person, string message)
{
    cout << ((person > 0) ? "customer[" : "barber [")
         << abs(person) << "]: " << message << endl;
}
