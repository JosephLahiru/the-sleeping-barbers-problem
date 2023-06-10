# Sleeping Barbers Problem

This repository contains a C++ implementation of the Sleeping Barbers problem, a classic synchronization and multithreading problem.

## Problem Description

The Sleeping Barbers problem involves a barber shop with a finite number of barbers and chairs. Customers visit the shop and wait in the available chairs if all barbers are busy. If there are no available chairs, the customer leaves. When a barber finishes serving a customer, they either serve the next waiting customer or sleep if there are no customers waiting.

The goal is to implement a solution that ensures proper synchronization between the barbers and customers, preventing deadlocks and race conditions.

## Implementation

The implementation consists of three files:

- `Shop.h`: Header file for the `Shop` class, which manages the synchronization between barbers and customers.
- `Shop.cpp`: Implementation of the `Shop` class methods.
- `driver.cpp`: The main driver program that creates barber and customer threads and simulates the interactions between them.

## How to Compile and Run

1. Save the three files as `driver.cpp`, `Shop.h`, and `Shop.cpp` in the same directory.
2. Open a terminal or command prompt, navigate to the directory containing the files.
3. Compile the files using the following command:

` g++ -o barber_shop driver.cpp Shop.cpp -lpthread`

This command compiles `driver.cpp` and `Shop.cpp` into an executable named `barber_shop` and links the pthread library, which is required for multithreading.

4. Run the compiled program with the required command-line arguments: the number of barbers, the number of chairs, the number of customers, and the service time (in microseconds). For example:

`./barber_shop 2 3 10 500000`

This command runs the program with 2 barbers, 3 chairs, 10 customers, and a service time of 500000 microseconds (0.5 seconds) for each customer.

Adjust the numbers as needed for your specific test case. The program should output the interactions between the customers and barbers as they visit the shop, wait for service, and leave after being served.
