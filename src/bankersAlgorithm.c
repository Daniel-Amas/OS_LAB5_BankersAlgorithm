#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define NUM_RESOURCES 3
#define NUM_CUSTOMERS 5

// Available amount of each resource
int resource_available[NUM_RESOURCES];
// Maximum demand of each customer
int customer_maximum[NUM_CUSTOMERS][NUM_RESOURCES];
// Amount currently allocated to each customer
int customer_allocation[NUM_CUSTOMERS][NUM_RESOURCES];
// Remaining need of each customer
int customer_need[NUM_CUSTOMERS][NUM_RESOURCES];

// Mutex lock
pthread_mutex_t lock;

// Function prototypes
int request_resources(int customer_id, int request[]);
int release_resources(int customer_id, int release[]);
bool is_system_safe();

// Request resources from the system
int request_resources(int customer_id, int request[]) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (request[i] > customer_need[customer_id][i] || request[i] > resource_available[i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        resource_available[i] -= request[i];
        customer_allocation[customer_id][i] += request[i];
        customer_need[customer_id][i] -= request[i];
    }

    if (!is_system_safe()) {
        // Roll back changes if unsafe state
        for (int i = 0; i < NUM_RESOURCES; i++) {
            resource_available[i] += request[i];
            customer_allocation[customer_id][i] -= request[i];
            customer_need[customer_id][i] += request[i];
        }
        pthread_mutex_unlock(&lock);
        return -1;
    }

    pthread_mutex_unlock(&lock);
    return 0;
}

// Release resources back to the system
int release_resources(int customer_id, int release[]) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        resource_available[i] += release[i];
        customer_allocation[customer_id][i] -= release[i];
        customer_need[customer_id][i] += release[i];
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

// Check if the system is in a safe state
bool is_system_safe() {
    int work[NUM_RESOURCES];
    bool finish[NUM_CUSTOMERS] = {false};

    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = resource_available[i];
    }

    int count = 0;
    while (count < NUM_CUSTOMERS) {
        bool found = false;
        for (int i = 0; i < NUM_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUM_RESOURCES; j++) {
                    if (customer_need[i][j] > work[j])
                        break;
                }
                if (j == NUM_RESOURCES) {
                    for (int x = 0; x < NUM_RESOURCES; x++) {
                        work[x] += customer_allocation[i][x];
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
    return count == NUM_CUSTOMERS;
}

// Thread function for customers
void *customer_thread(void *num) {
    int customer_id = *(int *)num;
    while (1) {
        int request[NUM_RESOURCES];
        pthread_mutex_lock(&lock);
        for (int i = 0; i < NUM_RESOURCES; i++) {
            request[i] = rand() % (customer_maximum[customer_id][i] + 1);
        }
        pthread_mutex_unlock(&lock);
        usleep(500000); // Sleep for a random amount of time (0.5 seconds)
        request_resources(customer_id, request);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // Initialize available resources from command line arguments
    if (argc != NUM_RESOURCES + 1) {
        printf("Usage: %s <resource 1> <resource 2> ... <resource n>\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        resource_available[i - 1] = atoi(argv[i]);
    }

    // Initialize maximum demand randomly
    srand(time(NULL));
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            customer_maximum[i][j] = rand() % (resource_available[j] + 1);
        }
    }

    // Initialize need and allocation matrices
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            customer_allocation[i][j] = 0;
            customer_need[i][j] = customer_maximum[i][j];
        }
    }

    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Create customer threads
    pthread_t threads[NUM_CUSTOMERS];
    int customers[NUM_CUSTOMERS];

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customers[i] = i;
        pthread_create(&threads[i], NULL, customer_thread, &customers[i]);
    }

    // Wait for a while
    sleep(10);

    // Terminate customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    return 0;
}
