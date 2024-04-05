#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> 
#include <pthread.h>
#include <signal.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

// Available amount of each resource
int available[NUMBER_OF_RESOURCES] = {10, 5, 7};
// Max demand of each customer
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
// Amount currently allocated to each customer
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
// Remaining need of each customer 
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

// Mutex lock
pthread_mutex_t mutex;

volatile sig_atomic_t running = 1;


// Function prototypes
int request(int customer_num, int request []); 
int release(int customer_num, int release []); 
bool safestate(int customer_num, int request []);
void* customer(void* num);
void calculateneed();

// Request resources from the system
int request_resources(int customer_num, int request []) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            printf("Error: request exceeds maximum claim\n");
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]){
            printf("Process %d must wait as the resources are not available\n", customer_num);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++){
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    } 
    if (!safestate(customer_num, request)) {
        pthread_mutex_unlock(&mutex);
        return -1;
    } 
    pthread_mutex_unlock(&mutex);
    return 0;
}

// Check if the system is in a safe state
bool safestate(int customer_num, int request []) { 
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS] = {false};
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i] - request[i];
    }
    
    int count = 0;
    while (count < NUMBER_OF_CUSTOMERS) {
        bool found = false;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if(need[i][j] > work[j])
                        break;
                }
                if (j == NUMBER_OF_RESOURCES) {
                    for (int x = 0; x < NUMBER_OF_RESOURCES; x++) {
                        work[x] += allocation[i][x];
                    }
                    finish[i] = true;
                    found = true;
                    count++;
                }
            }
        }
        if (!found)
            break;
    }
    return count == NUMBER_OF_CUSTOMERS;
}

// Release resources back to the system
int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}



// Calculate the remaining need of each customer
void calculateneed() {
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) { 
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }      
}  

// Thread function for customers
void* customer(void* num) {
    int customer_num = *(int*)num;
    while(running) {
        int request[NUMBER_OF_RESOURCES];
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            request[i] = rand() % (maximum[customer_num][i]+ 1);
            printf("Customer %d requested %d units of resource %d\n", customer_num, request[i], i);

        }
        pthread_mutex_unlock(&mutex);
       
        sleep(1);

        int result = request_resources(customer_num, request);
        if (result !=0){
            printf("request denied");
        }
    }
    return NULL;
}

int main()
{
    calculateneed();

    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[NUMBER_OF_CUSTOMERS];
    int customers[NUMBER_OF_CUSTOMERS];

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        customers[i] = i;
        if (pthread_create(&threads[i], NULL, customer, &customers[i]) != 0)
        {
            printf("Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Sleep for 10 seconds
    sleep(10);

    // Set the running flag to 0 to stop the threads
    running = 0;

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            printf("Error joining thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    pthread_mutex_destroy(&mutex);

    printf("Need matrix:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }

    int request_array[NUMBER_OF_RESOURCES] = {7, 4, 3};
    int customer_num = 0;

    int result = request_resources(customer_num, request_array);
    if (result == 0)
    {
        printf("Request granted\n");
    }

    bool safe = safestate(customer_num, request_array);
    if (safe)
    {
        printf("System is in a safe state\n");
    }
    else
    {
        printf("System is in an unsafe state\n");
    }

    return 0;
}