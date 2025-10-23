
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

static void *worker_thread_function(void *pool_ptr);

/**
 * Calculate work range for a thread in parallel processing
 * @param total_size Total number of elements to process
 * @param num_threads Number of threads
 * @param thread_id Current thread ID
 * @param start Output start index for this thread
 * @param end Output end index for this thread
 */
void get_thread_work_range(long total_size, int num_threads, int thread_id,
                           long *start, long *end) {
  long chunk_size = total_size / num_threads;
  *start = thread_id * chunk_size;
  *end = (thread_id == num_threads - 1) ? total_size : (*start) + chunk_size;
}

/**
 * Create a thread pool for parallel processing
 * @param num_threads Number of worker threads
 * @param queue_size Size of task queue
 * @return Pointer to created thread pool or NULL on failure
 */
thread_pool_t *thread_pool_create(int num_threads, int queue_size) {
  thread_pool_t *pool;
  int i;

  if (num_threads <= 0 || queue_size <= 0) {
    return NULL;
  }

  pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
  if (pool == NULL) {
    return NULL;
  }

  pool->num_threads = num_threads;
  pool->queue_size = queue_size;
  pool->head = pool->tail = pool->task_count = pool->active_tasks = 0;
  pool->shutdown = 0;

  pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  pool->task_queue =
      (struct t_task *)malloc(sizeof(struct t_task) * queue_size);

  if (pool->threads == NULL || pool->task_queue == NULL) {
    if (pool->threads)
      free(pool->threads);
    if (pool->task_queue)
      free(pool->task_queue);
    free(pool);
    return NULL;
  }

  pthread_mutex_init(&(pool->lock), NULL);
  pthread_cond_init(&(pool->notify), NULL);
  pthread_cond_init(&(pool->all_tasks_done), NULL);

  for (i = 0; i < num_threads; i++) {
    pthread_create(&(pool->threads[i]), NULL, worker_thread_function, pool);
  }

  return pool;
}

int thread_pool_add_task(thread_pool_t *pool, void (*function)(void *),
                         void *arg) {
  pthread_mutex_lock(&(pool->lock));

  if (pool->task_count == pool->queue_size) {
    pthread_mutex_unlock(&(pool->lock));
    fprintf(stderr, "Error: Thread pool task queue is full.\n");
    return -1;
  }

  pool->task_queue[pool->tail].function = function;
  pool->task_queue[pool->tail].argument = arg;
  pool->tail = (pool->tail + 1) % pool->queue_size;
  pool->task_count++;
  pool->active_tasks++;

  pthread_cond_broadcast(&(pool->notify));
  pthread_mutex_unlock(&(pool->lock));

  return 0;
}

/**
 * Wait for all tasks in thread pool to complete
 * @param pool Thread pool to wait for
 */
void thread_pool_wait(thread_pool_t *pool) {
  pthread_mutex_lock(&(pool->lock));

  while (pool->task_count > 0 || pool->active_tasks > 0) {
    pthread_cond_wait(&(pool->all_tasks_done), &(pool->lock));
  }

  pthread_mutex_unlock(&(pool->lock));
}

/**
 * Destroy thread pool and free all associated resources
 * @param pool Thread pool to destroy
 * @return 0 on success, -1 on failure
 */
int thread_pool_destroy(thread_pool_t *pool) {
  int i;

  pthread_mutex_lock(&(pool->lock));

  pool->shutdown = 1;

  pthread_cond_broadcast(&(pool->notify));
  pthread_mutex_unlock(&(pool->lock));

  for (i = 0; i < pool->num_threads; i++) {
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool->task_queue);

  pthread_mutex_destroy(&(pool->lock));
  pthread_cond_destroy(&(pool->notify));
  pthread_cond_destroy(&(pool->all_tasks_done));

  free(pool);

  return 0;
}

/**
 * Worker thread function that processes tasks from the queue
 * @param pool_ptr Pointer to thread pool
 * @return NULL on exit
 */
static void *worker_thread_function(void *pool_ptr) {
  thread_pool_t *pool = (thread_pool_t *)pool_ptr;
  struct t_task task;

  while (1) {
    pthread_mutex_lock(&(pool->lock));

    while (pool->task_count == 0 && !pool->shutdown) {
      pthread_cond_wait(&(pool->notify), &(pool->lock));
    }

    if (pool->shutdown) {
      pthread_mutex_unlock(&(pool->lock));
      break;
    }

    task.function = pool->task_queue[pool->head].function;
    task.argument = pool->task_queue[pool->head].argument;

    pool->head = (pool->head + 1) % pool->queue_size;
    pool->task_count--;

    pthread_mutex_unlock(&(pool->lock));

    (*(task.function))(task.argument);

    pthread_mutex_lock(&(pool->lock));

    pool->active_tasks--;

    if (pool->active_tasks == 0 && pool->task_count == 0) {
      pthread_cond_signal(&(pool->all_tasks_done));
    }

    pthread_mutex_unlock(&(pool->lock));
  }

  pthread_exit(NULL);
  return NULL;
}
