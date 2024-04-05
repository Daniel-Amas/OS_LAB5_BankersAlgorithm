#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define NUM_CUSTOMERS 5
#define NUM_RESOURCES 3

// Available amount of each resource
int available[NUM_RESOURCES];
// Max demand of each customer
int max_demand[NUM_CUSTOMERS][NUM_RESOURCES];
// Amount currently allocated to each customer
int allocation[NUM_CUSTOMERS][NUM_RESOURCES];
// Remaining need of each customer
int need[NUM_CUSTOMERS][NUM_RESOURCES];

// Mutex lock
pthread_mutex_t mutex;

// Function prototypes
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
bool is_safe_state();

// Request resources from the system
int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (request[i] > need[customer_num][i] || request[i] > available[i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    if (!is_safe_state()) {
        // Roll back changes if the state becomes unsafe
        for (int i = 0; i < NUM_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    printf("Request granted. Customer %d's allocation:", customer_num);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        printf(" %d", allocation[customer_num][i]);
    }
    printf("\n");

    pthread_mutex_unlock(&mutex);
    return 0;
}

// Release resources back to the system
int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }
    printf("Resources released. Customer %d's allocation:", customer_num);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        printf(" %d", allocation[customer_num][i]);
    }
    printf("\n");

    pthread_mutex_unlock(&mutex);
    return 0;
}

// Check if the system is in a safe state
bool is_safe_state() {
    int work[NUM_RESOURCES];
    bool finish[NUM_CUSTOMERS] = {false};

    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = available[i];
    }

    int count = 0;
    while (count < NUM_CUSTOMERS) {
        bool found = false;
        for (int i = 0; i < NUM_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUM_RESOURCES; j++) {
                    if (need[i][j] > work[j])
                        break;
                }
                if (j == NUM_RESOURCES) {
                    for (int x = 0; x < NUM_RESOURCES; x++) {
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
    return count == NUM_CUSTOMERS;
}

// Thread function for customers
void *customer(void *num) {
    int customer_num = *(int *)num;
    while (1) {
        int request[NUM_RESOURCES];
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < NUM_RESOURCES; i++) {
            request[i] = rand() % (max_demand[customer_num][i] + 1);
        }
        pthread_mutex_unlock(&mutex);
        usleep(500000); // Sleep for a random amount of time (0.5 seconds)
        printf("Customer %d requests:", customer_num);
        for (int i = 0; i < NUM_RESOURCES; i++) {
            printf(" %d", request[i]);
        }
        printf("\n");
        request_resources(customer_num, request);
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
        available[i - 1] = atoi(argv[i]);
    }

    // Initialize max demand randomly
    srand(time(NULL));
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            max_demand[i][j] = rand() % (available[j] + 1);
        }
    }

    // Initialize need and allocation matrices
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            allocation[i][j] = 0;
            need[i][j] = max_demand[i][j];
        }
    }

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create customer threads
    pthread_t threads[NUM_CUSTOMERS];
    int customers[NUM_CUSTOMERS];

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customers[i] = i;
        pthread_create(&threads[i], NULL, customer, &customers[i]);
    }

    // Wait for a while
    sleep(10);

    // Terminate customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    return 0;
}
