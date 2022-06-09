#include "MapReduceFramework.h"
#include "Barrier.h"
#include <pthread.h>
#include <vector>
#include <atomic>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>

using std::string;
using std::cerr;
using std::endl;
using std::map;
typedef std::pair<pthread_t *, IntermediateVec *> thread_to_vecPair;

// error handling function
void handleError (const string &error_message)
{
  cerr << "system error: " << error_message << endl;
  exit (EXIT_FAILURE);
}

typedef struct JobContext {
    int num_of_threads;
    pthread_t *threads;
    map<pthread_t *, IntermediateVec *> &thread_to_vec; // each thread has its own intermediate vector
    std::vector<IntermediateVec> &queue_vec;

    //parameters for the jobs
    const InputVec &input_vec;
    const MapReduceClient &client;
    OutputVec &output_vec;

    //synchronization tools
    Barrier *barrier;
    std::atomic<uint64_t> atomic_counter;

    pthread_mutex_t *mutex_map;
    pthread_mutex_t *mutex_reduce;
    pthread_mutex_t *mutex_emit3;
    pthread_mutex_t *mutex_wait;

    // status of the job
    bool waiting;

} JobContext;

void lock (pthread_mutex_t *mutex)
{
  if (pthread_mutex_lock (mutex) != 0)
    {
      handleError ("lock mutex failed");
    }
}

void unlock (pthread_mutex_t *mutex)
{
  if (pthread_mutex_unlock (mutex) != 0)
    {
      handleError ("unlock mutex failed");
    }
}

unsigned long get_stage (unsigned long counter)
{
  return counter >> 62;
}

unsigned long get_total (unsigned long counter)
{
  return (counter << 2) >> 33;
}

unsigned long get_index (unsigned long counter)
{
  return (counter << 33) >> 33;
}

void init_atomic (JobContext *jc, unsigned long stage, unsigned long total_size)
{
  jc->atomic_counter = ((uint64_t) total_size << 31) | ((uint64_t) stage << 62);
}

void mapPhase (JobContext *jc, uint64_t &old_value)
{

  lock (jc->mutex_map);
  if (get_stage (jc->atomic_counter) == UNDEFINED_STAGE)
    {
      init_atomic (jc, MAP_STAGE, jc->input_vec.size ());
    }
  old_value = jc->atomic_counter++;
  old_value = get_index (old_value);
  unlock (jc->mutex_map);
  while (true)
    {
      if (old_value < jc->input_vec.size ())
        {
          InputPair pair = jc->input_vec[old_value];
          jc->client.map (pair.first, pair.second, jc);
          old_value = jc->atomic_counter++;
          old_value = get_index (old_value);
        }
      else
        {
          (jc->atomic_counter)--;
          break;
        }
    }

}

bool comparator (IntermediatePair &a, IntermediatePair &b)
{
  return ((*a.first) < (*b.first));
}

void sortPhase (JobContext *jc)
{
  pthread_t *found;

  for (auto &pair: jc->thread_to_vec)
    {
      if (pthread_self () == *pair.first)
        {
          found = pair.first;
        }
    }

  auto *vec_to_sort = jc->thread_to_vec[found];
  sort (vec_to_sort->begin (), vec_to_sort->end (), comparator);
}

// finds the pair with the max key
K2 *findMax (JobContext *jc)
{
  K2 *cur_max = nullptr;

  for (auto &item: jc->thread_to_vec)
    {
      auto curr_vec = item.second;
      if (curr_vec->empty ())
        {
          continue;
        }
      K2 *max_key_vec = curr_vec->back ().first;
      if (cur_max == nullptr || *cur_max < *max_key_vec)
        {
          cur_max = max_key_vec;
        }
    }

  return cur_max;
}

void prepareCounterForShuffle (JobContext *jc)
{
  unsigned long num_of_pairs = 0;
  for (auto &t: jc->thread_to_vec)
    {
      num_of_pairs += t.second->size ();
    }
  init_atomic (jc, SHUFFLE_STAGE, num_of_pairs);
}

void prepareCounterForReduce (JobContext *jc)
{
  unsigned long num_of_pairs = 0;
  for (auto &t: jc->queue_vec)
    {
      num_of_pairs += t.size ();
    }
  init_atomic (jc, REDUCE_STAGE, num_of_pairs);
}

bool isEqualKeys (K2 *max, IntermediateVec *curr_vec)
{
  return (!(*max < *(curr_vec->back ().first)) && !(*(curr_vec->back ().first) < *max));
}

void shufflePhase (JobContext *jc){
  K2 *max = findMax (jc);
  while (max != nullptr){
      IntermediateVec new_vector;
      for (auto &item: jc->thread_to_vec){
          auto curr_vec = item.second;
          while (!curr_vec->empty () && isEqualKeys (max, curr_vec)){
              new_vector.push_back (curr_vec->back ());
              curr_vec->pop_back ();
              (jc->atomic_counter)++;
            }
        }
      if (!new_vector.empty ()){
          jc->queue_vec.push_back (new_vector);
        }
      max = findMax (jc);
    }

}

void reducePhase (JobContext *jc)
{

  while (true)
    {
      lock (jc->mutex_reduce);
      if (jc->queue_vec.empty ())
        {
          pthread_mutex_unlock (jc->mutex_reduce);
          break;
        }
      auto cur_vec = jc->queue_vec.back ();
      jc->queue_vec.pop_back ();
      unlock (jc->mutex_reduce);

      jc->client.reduce (&cur_vec, jc);
      (jc->atomic_counter) += (cur_vec.size ());
    }
}

// the function that each thread is running
void *threadJob (void *arg)
{
  auto *jc = (JobContext *) arg;
  uint64_t old_value;
  mapPhase (jc, old_value);
  sortPhase (jc);
  jc->barrier->barrier ();
  if (pthread_self () == jc->threads[0])
    {
      prepareCounterForShuffle (jc);
      shufflePhase (jc);
      prepareCounterForReduce (jc);
    }
  jc->barrier->barrier ();
  reducePhase (jc);
  return nullptr;
}

void emit2 (K2 *key, V2 *value, void *context)
{
  auto jc = (JobContext *) context;
  pthread_t *found;

  for (auto &pair: jc->thread_to_vec)
    {
      if (pthread_self () == *pair.first)
        {
          found = pair.first;
        }
    }
  jc->thread_to_vec[found]->push_back (IntermediatePair (key, value));
}

void emit3 (K3 *key, V3 *value, void *context)
{
  auto jc = (JobContext *) context;
  lock (jc->mutex_emit3);
  jc->output_vec.push_back (OutputPair (key, value));
  unlock (jc->mutex_emit3);
}

JobHandle startMapReduceJob (const MapReduceClient &client,
                             const InputVec &inputVec, OutputVec &outputVec,
                             int multiThreadLevel)
{

  //allocate dynamic databases
  auto thread_to_vec = new map<pthread_t *, IntermediateVec *>;
  auto queue_vec = new std::vector<IntermediateVec>;
  auto threads = new pthread_t[multiThreadLevel];
  auto barrier = new Barrier (multiThreadLevel);
  // init mutexes
  auto *mutex_map = new pthread_mutex_t;
  pthread_mutex_init (mutex_map, nullptr);
  auto *mutex_reduce = new pthread_mutex_t;
  pthread_mutex_init (mutex_reduce, nullptr);
  auto *mutex_emit3 = new pthread_mutex_t;
  pthread_mutex_init (mutex_emit3, nullptr);
  auto *mutex_wait = new pthread_mutex_t;
  pthread_mutex_init (mutex_wait, nullptr);

  // init all JobContext elements
  auto *job_context = new JobContext{
      .num_of_threads = multiThreadLevel,
      .threads = threads,
      .thread_to_vec = *thread_to_vec,
      .queue_vec = *queue_vec,
      .input_vec = inputVec,
      .client = client,
      .output_vec = outputVec,

      .barrier = barrier,
      .atomic_counter{0},

      .mutex_map = mutex_map,
      .mutex_reduce = mutex_reduce,
      .mutex_emit3 = mutex_emit3,
      .mutex_wait = mutex_wait,

      .waiting = false,
  };

  // create intermediate vectors for each thread
  for (int i = 0; i < multiThreadLevel; ++i)
    {
      thread_to_vec->insert (thread_to_vecPair (&threads[i], new IntermediateVec ()));
    }
  // create threads
  for (int i = 0; i < multiThreadLevel; i++)
    {
      if (pthread_create (&(job_context->threads[i]), nullptr, threadJob, job_context)
          != 0)
        {
          handleError ("error with creating thread");
        }
    }
  return job_context;
}

void waitForJob (JobHandle job)
{
  auto *j = (JobContext *) job;
  lock (j->mutex_wait);
  if (!j->waiting)
    {
      pthread_t *threads = j->threads;
      for (int i = 0; i < j->num_of_threads; ++i)
        {
          if (pthread_join (threads[i], nullptr) != 0)
            {
              handleError ("error in pthread join");
            };
        }
      j->waiting = true;
    }
  unlock (j->mutex_wait);

}

void getJobState (JobHandle job, JobState *state)
{
  auto *jc = (JobContext *) job;
  auto counter = jc->atomic_counter.load ();

  state->stage = (stage_t) get_stage (counter);
  auto total = get_total (counter);
  auto done = get_index (counter);
  if (total == 0)
    {
      state->percentage = 0;
      return;
    }
  state->percentage = std::min (((float) done / (float) total) * 100, float (100));
}

void closeJobHandle (JobHandle job)
{
  waitForJob (job);
  auto *jc = (JobContext *) job;

  for (auto &pair: jc->thread_to_vec)
    {
      delete pair.second;
    }
  jc->thread_to_vec.clear ();
  delete &jc->thread_to_vec;

  delete jc->barrier;

  jc->queue_vec.clear ();

  delete &jc->queue_vec;

  if (pthread_mutex_destroy (jc->mutex_map) != 0)
    {
      handleError ("error in destroy mutex map");
    };
  if (pthread_mutex_destroy (jc->mutex_wait) != 0)
    {
      handleError ("error in destroy mutex wait");
    }
  if (pthread_mutex_destroy (jc->mutex_reduce) != 0)
    {
      handleError ("error in destroy mutex reduce");
    }
  if (pthread_mutex_destroy (jc->mutex_emit3) != 0)
    {
      handleError ("error in destroy mutex emit3");
    }

  delete jc->mutex_map;
  delete jc->mutex_reduce;
  delete jc->mutex_wait;
  delete jc->mutex_emit3;

  delete[] jc->threads;
  delete jc;
  jc = nullptr;
  job = nullptr;
}




