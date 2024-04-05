#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
pthread_mutex_t mutex;

// Function prototypes
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
int safety_check(int customer_num, int request[]);

// Request resources from the bank
int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&mutex);

    // Check if request can be satisfied
    if (!safety_check(customer_num, request)) {
        pthread_mutex_unlock(&mutex);
        return -1; // Request cannot be granted
    }

    // Allocate resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    pthread_mutex_unlock(&mutex);
    return 0; // Request granted
}

// Release resources back to the bank
int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);

    // Release resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

// Check if the system is in a safe state after granting a request
int safety_check(int customer_num, int request[]) {
    // Simulate allocation for safety check
    int temp_available[NUMBER_OF_RESOURCES];
    int temp_allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
    int temp_need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        temp_available[i] = available[i] - request[i];
        temp_allocation[customer_num][i] = allocation[customer_num][i] + request[i];
        temp_need[customer_num][i] = need[customer_num][i] - request[i];
    }

    int work[NUMBER_OF_RESOURCES];
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        work[i] = temp_available[i];

    int finish[NUMBER_OF_CUSTOMERS] = {0};
    int safe_sequence[NUMBER_OF_CUSTOMERS];
    int count = 0;

    while (count < NUMBER_OF_CUSTOMERS) {
        int found = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (temp_need[i][j] > work[j])
                        break;
                }
                if (j == NUMBER_OF_RESOURCES) {
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++)
                        work[k] += temp_allocation[i][k];
                    safe_sequence[count++] = i;
                    finish[i] = 1;
                    found = 1;
                }
            }
        }
        if (!found)
            break;
    }

    return count == NUMBER_OF_CUSTOMERS;
}

void *customer_thread(void *arg) {
    int customer_num = *(int *)arg;

    while (1) {
        // Generate random request
        int request[NUMBER_OF_RESOURCES];
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            request[i] = rand() % (need[customer_num][i] + 1);
        }

        // Request resources
        request_resources(customer_num, request);

        // Simulate some work
        sleep(1);

        // Generate random release
        int release[NUMBER_OF_RESOURCES];
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            release[i] = rand() % (allocation[customer_num][i] + 1);
        }

        // Release resources
        release_resources(customer_num, release);

        // Simulate some work
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    // Initialize available resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }

    // Initialize maximum demand for each customer (can be done differently)
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create customer threads
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    int customer_ids[NUMBER_OF_CUSTOMERS];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        customer_ids[i] = i;
        pthread_create(&customers[i], NULL, customer_thread, &customer_ids[i]);
    }

    // Join customer threads
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}
