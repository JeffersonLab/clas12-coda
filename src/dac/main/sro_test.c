
/* sro_test.c */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_SENDERS 2
#define NUM_RECEIVERS 4
#define ITEMS_PER_SENDER 5

// Shared work unit
typedef struct {
    int sender_id;
    int data_value;
} WorkItem;

// Shared state for round-robin distribution
typedef struct {
    WorkItem queue[NUM_RECEIVERS];
    int has_data[NUM_RECEIVERS]; // Flag per receiver thread
    int next_receiver;           // Round-robin index (0-3)
    pthread_mutex_t lock;
    pthread_cond_t cond[NUM_RECEIVERS];
} RoundRobinRouter;

RoundRobinRouter router;







// Sender Thread Function
void *
sender_routine(void* arg)
{
  int sender_id = *(int*)arg;
  free(arg);

  for (int i = 0; i < ITEMS_PER_SENDER; i++)
  {
    int target_receiver;

    pthread_mutex_lock(&router.lock);

    // Wait if the target receiver's slot is still occupied
    while (router.has_data[router.next_receiver])
    {
      // For simplicity, we spin or you can manage separate queues per receiver.
      // Here we wait until the specific slot is clear.
      pthread_mutex_unlock(&router.lock);
      usleep(1000);
      pthread_mutex_lock(&router.lock);
    }

    // Assign round-robin target
        target_receiver = router.next_receiver;
        
        // Populate data for the chosen receiver
        router.queue[target_receiver].sender_id = sender_id;
        router.queue[target_receiver].data_value = (sender_id * 100) + i;
        router.has_data[target_receiver] = 1;

        printf("[Sender %d] Sent data %d to Receiver %d\n", 
               sender_id, router.queue[target_receiver].data_value, target_receiver);

        // Signal the specific receiver
        pthread_cond_signal(&router.cond[target_receiver]);

        // Advance round-robin index to the next worker (0 -> 1 -> 2 -> 3 -> 0)
        router.next_receiver = (router.next_receiver + 1) % NUM_RECEIVERS;

        pthread_mutex_unlock(&router.lock);
        usleep(50000); // simulate work/delay
    }
    return NULL;
}

// Receiver Thread Function
void* receiver_routine(void* arg) {
    int receiver_id = *(int*)arg;
    free(arg);

    for (int i = 0; i < (NUM_SENDERS * ITEMS_PER_SENDER) / NUM_RECEIVERS; i++) {
        pthread_mutex_lock(&router.lock);

        // Wait until data is available for this specific receiver
        while (!router.has_data[receiver_id]) {
            pthread_cond_wait(&router.cond[receiver_id], &router.lock);
        }

        // Consume data
        WorkItem item = router.queue[receiver_id];
        router.has_data[receiver_id] = 0; // Clear slot

        printf("  [Receiver %d] Processed data %d from Sender %d\n", 
               receiver_id, item.data_value, item.sender_id);

        pthread_mutex_unlock(&router.lock);
    }
    return NULL;
}

int
main(void)
{

  
  pthread_t senders[NUM_SENDERS];
  pthread_t receivers[NUM_RECEIVERS];


    
  // Initialize router state
  router.next_receiver = 0;
  pthread_mutex_init(&router.lock, NULL);
  for (int i = 0; i < NUM_RECEIVERS; i++) {
        router.has_data[i] = 0;
        pthread_cond_init(&router.cond[i], NULL);
    }




    
    // Create 4 receivers
    for (int i = 0; i < NUM_RECEIVERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&receivers[i], NULL, receiver_routine, id);
    }

    // Create 2 senders
    for (int i = 0; i < NUM_SENDERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&senders[i], NULL, sender_routine, id);
    }

    // Join senders
    for (int i = 0; i < NUM_SENDERS; i++) {
        pthread_join(senders[i], NULL);
    }

    // Join receivers
    for (int i = 0; i < NUM_RECEIVERS; i++) {
        pthread_join(receivers[i], NULL);
    }

    // Cleanup
    pthread_mutex_destroy(&router.lock);
    for (int i = 0; i < NUM_RECEIVERS; i++) {
        pthread_cond_destroy(&router.cond[i]);
    }

    return 0;
}
