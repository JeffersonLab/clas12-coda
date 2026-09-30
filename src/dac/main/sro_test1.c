
/* sro_test1.c */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_PRODUCERS 2
#define NUM_WORKERS 4
#define ITEMS_PER_PRODUCER 10

// Queue Node
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Private Queue with its own synchronization primitives
typedef struct {
    Node* head;
    Node* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
} Queue;

// Global structures
Queue queues[NUM_WORKERS];
bool shutdown_flag = false;
pthread_mutex_t global_counter_mutex;
int global_sequence = 0;



// Helper to push to a specific queue
void
queue_push(Queue* q, int data)
{
  Node* new_node = (Node*)malloc(sizeof(Node));
  new_node->data = data;
  new_node->next = NULL;

  pthread_mutex_lock(&q->mutex);
  if (q->tail == NULL)
  {
    q->head = q->tail = new_node;
  }
  else
  {
    q->tail->next = new_node;
    q->tail = new_node;
  }
  
  q->count++;
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);
}



// Helper to pop from a specific queue
bool
queue_pop(Queue* q, int* data)
{
  pthread_mutex_lock(&q->mutex);
  while (q->head == NULL && !shutdown_flag)
  {
    pthread_cond_wait(&q->cond, &q->mutex);
  }
  if (q->head == NULL && shutdown_flag)
  {
    pthread_mutex_unlock(&q->mutex);
    return false;
  }
  
  Node* temp = q->head;
  *data = temp->data;
  q->head = temp->next;
  if (q->head == NULL)
  {
    q->tail = NULL;
  }
  q->count--;
  pthread_mutex_unlock(&q->mutex);
  free(temp);
  
  return true;
}



// Producer Thread Function
void *
producer_func(void* arg)
{
  int id = *(int*)arg;
  free(arg);

  for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
  {
    pthread_mutex_lock(&global_counter_mutex);
    int target_queue = global_sequence % NUM_WORKERS;
    global_sequence++;
    pthread_mutex_unlock(&global_counter_mutex);

    int item_data = id * 100 + i;
    queue_push(&queues[target_queue], item_data);
    usleep(10000); // Simulate work
  }
  return NULL;
}




// Worker Thread Function
void *
worker_func(void* arg)
{
  int id = *(int*)arg;
  free(arg);

  int data;
  while (true)
  {
    if (!queue_pop(&queues[id], &data))
    {
      break; // Exit if shutdown and queue empty
    }
    printf("Worker %d processed data: %d\n", id, data);
  }
  return NULL;
}


int
main(void)
{
    pthread_t producers[NUM_PRODUCERS];
    pthread_t workers[NUM_WORKERS];

    // Initialize queues, mutexes, and condition variables
    pthread_mutex_init(&global_counter_mutex, NULL);
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        queues[i].head = NULL;
        queues[i].tail = NULL;
        queues[i].count = 0;
        pthread_mutex_init(&queues[i].mutex, NULL);
        pthread_cond_init(&queues[i].cond, NULL);
    }

    // Create workers
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&workers[i], NULL, worker_func, id);
    }

    // Create producers
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&producers[i], NULL, producer_func, id);
    }




    
    // Wait for producers to finish
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    // Signal shutdown and wake up sleeping workers
    pthread_mutex_lock(&global_counter_mutex);
    shutdown_flag = true;
    pthread_mutex_unlock(&global_counter_mutex);

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_mutex_lock(&queues[i].mutex);
        pthread_cond_broadcast(&queues[i].cond);
        pthread_mutex_unlock(&queues[i].mutex);
    }

    // Wait for workers to finish
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }

    // Cleanup resources
    pthread_mutex_destroy(&global_counter_mutex);
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_mutex_destroy(&queues[i].mutex);
        pthread_cond_destroy(&queues[i].cond);
    }

    return 0;
}

