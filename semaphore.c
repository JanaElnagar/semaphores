#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

#define N 5     // Number of mCounter threads
#define B 3     // Buffer size

// Structure for a FIFO queue node
typedef struct node {
    int data;
    struct node *next;
} Node;

// Global variables
int counter = 0;
Node *head = NULL, *tail = NULL;  // FIFO queue pointers
//int buffer[B];
int writeIndex = 0, readIndex = 0;

sem_t counterMutex, bufferMutex, empty, full;

// Function prototypes
void enqueue(int value);
int dequeue();
void* mCounterThread(void* threadID);
void* mMonitorThread(void* arg);
void* mCollectorThread(void* arg);
void intHandler(int dummy);

int main() {
    signal(SIGINT, intHandler);

    pthread_t mCounterThreads[N];
    pthread_t mMonitorThreadID, mCollectorThreadID;
    
    // Initialize semaphores
    sem_init(&counterMutex, 0, 1);
    sem_init(&bufferMutex, 0, 1);
    sem_init(&empty, 0, B);
    sem_init(&full, 0, 0);
    
    // Create mCounter threads
    for (int i = 0; i < N; i++) {
        pthread_create(&mCounterThreads[i], NULL, mCounterThread, (void*)(long)i);
    }

    
    pthread_create(&mCollectorThreadID, NULL, mCollectorThread, NULL);
    pthread_create(&mMonitorThreadID, NULL, mMonitorThread, NULL);
    
     
    // Join mCounter threads
    for (int i = 0; i < N; i++) {
        pthread_join(mCounterThreads[i], NULL);
    }
    
    // Join threads
    pthread_join(mMonitorThreadID, NULL);
    pthread_join(mCollectorThreadID, NULL);
    
    // Destroy semaphores
    sem_destroy(&counterMutex);
    sem_destroy(&bufferMutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    
    return 0;
}

void* mCounterThread(void* threadID) {
    //for (int i =1; i<= N;i++){
        int id = (int)(long)threadID;
        
        //srandom((unsigned int)id);
        sleep(random() % 5 + 1);
        
        printf("Counter thread %d: received a message\n", id);

        printf("Counter thread %d: waiting to write\n", id);
        // Request access to the counter
        sem_wait(&counterMutex);
        
        // Critical section
        counter++;
        printf("Counter thread %d: now adding to counter, counter value=%d\n", id, counter);

        // Release access to the counter
        sem_post(&counterMutex);
    //}
    return NULL;
}

void* mMonitorThread(void* arg) {
    int counterValue,sem_value;
    
    for (int i =1; i<= N;i++){
        // Sleep for a random interval
        //srandom((unsigned int)time(NULL));
        sleep(random() % 5 + 1); 

        // Request access to the buffer
        sem_wait(&bufferMutex);

        sem_getvalue(&full, &sem_value); //get value of full semaphore
        if (sem_value >= B) {
            printf("Monitor thread: Buffer full!! \n\n");
        }
        else{

            printf("Monitor thread: waiting to read counter\n");

            // Request access to the counter
            sem_wait(&counterMutex);
            
            // Critical section: Read counter value and reset counter
            counterValue = counter;
            counter = 0;
            printf("Monitor thread: reading a count value of %d\n", counterValue);

            // Release access to the counter
            sem_post(&counterMutex);
 
            sem_wait(&empty);
            
            // Critical section: Write counter value to the buffer 
            enqueue(counterValue);
            printf("Monitor thread: writing to buffer at position %d\n",writeIndex);
            writeIndex = (writeIndex + 1) % B;
            
            
            sem_post(&full);
        }
        // Release access to the buffer
        sem_post(&bufferMutex);
    }
    
    return NULL;
}

void* mCollectorThread(void* arg) {
    int counterValue, sem_value;
    
    for (int i =1; i<= N;i++){
        // Sleep for a random interval
        srandom((unsigned int)time(NULL));
        sleep(random() % 5 + 1);
        
        // Request access to the buffer
        sem_wait(&bufferMutex);

        sem_getvalue(&empty, &sem_value); //get value of empty semaphore
        if (sem_value >= B) {
            printf("Collector thread: nothing is in the buffer! \n\n");
        }
        else{
            
            sem_wait(&full);
            
            // Critical section: Read from buffer 
            counterValue = dequeue();
            printf("Collector thread: reading from the buffer at position %d\n", readIndex);
            readIndex = (readIndex + 1) % B; 
    
            sem_post(&empty);
        }
        // Release access to the buffer
        sem_post(&bufferMutex);

    }
    
    return NULL;
}

// Functions for FIFO queue 
void enqueue(int value) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->data = value;
    newNode->next = NULL;
    
    if (tail == NULL) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
}

int dequeue() {
    if (head == NULL) {
        // Handle empty queue condition
        printf("Error: Queue is empty\n");
        return -1;  // Or handle appropriately
    }
    
    Node *temp = head;
    int value = temp->data;
    head = head->next;
    
    if (head == NULL) {
        tail = NULL;
    }
    
    free(temp);
    return value;
}

void intHandler(int dummy) {
    printf("Exit\n");
	// Destroy the semaphore 
	sem_destroy(&counterMutex);
    sem_destroy(&bufferMutex);
    sem_destroy(&empty);
    sem_destroy(&full);
	exit(0);
}
