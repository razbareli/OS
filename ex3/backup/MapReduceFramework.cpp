#include "MapReduceFramework.h"
#include "Barrier/Barrier.h"
#include <pthread.h>
#include <vector>
#include <atomic>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>

#define FULL_31_BITS (pow(2, 32) - 1)

using namespace std;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;  // todo delete

typedef std::pair<pthread_t, IntermediateVec *> thread_to_vecPair;

void handleError(const string &error_message) {
    cerr << "system error: " << error_message << endl;
    exit(EXIT_FAILURE);
}

unsigned long get_index(unsigned long counter) {
    const auto on_32_bit = (int64_t) pow(2, 31);
    return counter % on_32_bit;
}

unsigned long get_stage(unsigned long counter) {
    return counter >> 62;
}

unsigned long get_size(unsigned long counter) {
    return (counter << 2) >> 33;
}

typedef struct JobContext {
    int num_of_threads;
    pthread_t *threads;
    bool first_thread_created;
    pthread_t first_thread;
    map<pthread_t,
            IntermediateVec *> &thread_to_vec; // each thread has its own intermediate vector
    vector<IntermediateVec *> &queue_vec;

    //parameters for the jobs
    const InputVec &input_vec;
    const MapReduceClient &client;
    OutputVec &output_vec;

    //synchronization tools
    Barrier *barrier;
    atomic<uint64_t> atomic_counter;
    atomic<uint64_t> atomic_percentage;
    pthread_mutex_t mutex_output_vec;
    pthread_mutex_t mutex_map;
    pthread_mutex_t mutex_reduce;
//    pthread_mutex_t mutex_wait_for_job;
//    pthread_mutex_t mutex_get_state;
    pthread_mutex_t mutex_begin;

    // status of the job
    bool waiting;

} JobContext;

void init_atomic_var(stage_t stage, JobContext *jc, unsigned long total_size) {
    uint64_t casted_stage = stage;
    casted_stage = casted_stage << 62;
    //  uint64_t num_of_pairs = jc->input_vec.size () << 31;
    uint64_t num_of_pairs = total_size << 31;
    jc->atomic_counter = num_of_pairs | casted_stage;
    jc->atomic_percentage = 0;
}

void lock(pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
}

void unlock(pthread_mutex_t *mutex) {
    pthread_mutex_unlock(mutex);
}

void mapPhase(JobContext *jc) {

    size_t input_vec_size = jc->input_vec.size();
    bool keepOnMapping = true;
    while (keepOnMapping) {
        lock (&jc->mutex_map);
        uint64_t old_value = (jc->atomic_counter)++;
        unlock (&jc->mutex_map);
        unsigned long index = get_index(old_value);
        if (index < input_vec_size) {
//            cout << "I'm thread " << pthread_self() % 1000 << " in Map" << endl;
//            cout << "index = " << index << endl;

            InputPair pair = jc->input_vec[get_index(index)];
            jc->client.map(pair.first, pair.second, jc);
            jc->atomic_percentage++;


        } else {
            keepOnMapping = false;
            (jc->atomic_counter)--;
        }

    }

}

void print_vec(IntermediateVec &vec) {
    for (auto &i: vec) {
//        cout << '(' << i.first << ',' << i.second << ')' << endl;
    }
}

void print_vec(const InputVec &vec) {
    for (const auto &i: vec) {
//        cout << '(' << i.first << ',' << i.second << ')' << endl;
    }
}

bool comparator(IntermediatePair &a, IntermediatePair &b) {
    return ((*a.first) < (*b.first));
}

bool equals(IntermediatePair &a, IntermediatePair &b) {
    if (!((*a.first) < (*b.first)) && !((*b.first) < (*a.first))) {
        return true;
    }
    return false;
}

void sortPhase(JobContext *jc) {
    auto *vev_to_sort = jc->thread_to_vec[pthread_self()];
    sort(vev_to_sort->begin(), vev_to_sort->end(), comparator);
    //  print_vec(*vev_to_sort);
}

K2* findMax(JobContext *jc) {
  K2 *cur_max = nullptr;

  for (auto &item: jc->thread_to_vec) {
    auto curr_vec = item.second;
    if (curr_vec->empty()) {
      continue;
    }
    K2 *max_key_vec = curr_vec->back().first;
    if(cur_max == nullptr ||  *cur_max < *max_key_vec){
      cur_max = max_key_vec;
    }
  }

  return cur_max;
}

void shufflePhase(JobContext *jc) {
//  lock (&jc->mutex_get_state);
  unsigned long num_of_pairs = 0;
  for (auto &t : jc->thread_to_vec) {
    num_of_pairs += t.second->size();
  }
  //  cout << "num_of_pairs: " << num_of_pairs << endl;

  init_atomic_var(SHUFFLE_STAGE, jc, num_of_pairs);
//  unlock (&jc->mutex_get_state);

  K2* max = findMax(jc);

  while (max != nullptr){
    auto new_vector =  new IntermediateVec();
    for (auto &item: jc->thread_to_vec) {
      auto curr_vec = item.second;
      if (curr_vec->empty()) {
        continue;
      }
      if (!curr_vec->empty() && (!(*max < *(curr_vec->back().first)) && !(*(curr_vec->back().first) < *max)))
      {
        new_vector->push_back(curr_vec->back());
        curr_vec->pop_back();
        (jc->atomic_counter)++;
        jc->atomic_percentage++;
      }
    }
    if (!new_vector->empty()){
      jc->queue_vec.push_back(new_vector);
    }
    max = findMax(jc);
  }
//  lock (&jc->mutex_get_state);

  init_atomic_var(REDUCE_STAGE, jc, jc->queue_vec.size());
//  unlock (&jc->mutex_get_state);

}
//
//void shufflePhase(JobContext *jc) {
////    cout << "I'm thread " << pthread_self() % 1000 << " in shuffle" << endl;
//
//    unsigned long num_of_pairs = 0;
//    for (auto &t: jc->thread_to_vec) {
////        cout << "vec";
//        num_of_pairs += t.second->size();
//    }
////    cout << num_of_pairs;
//    init_atomic_var(SHUFFLE_STAGE, jc, num_of_pairs);
//    unsigned long pairs_shuffled = 0;
//    while (pairs_shuffled < num_of_pairs) {
//        // first, find the largest key in the data
//        K2* curr_max_key = nullptr;
//        for (auto &item: jc->thread_to_vec) {
//            auto curr_vec = item.second;
//            if (curr_vec->empty()) {
//                continue;
//            }
//            K2* curr_key = curr_vec->back().first;
//            if (curr_max_key == nullptr || *curr_max_key < *curr_key) {
//                curr_max_key = curr_key;
//            }
//        }
//        // create a new vector for this key
//        auto new_vector = new IntermediateVec();
//        // iterate over all the pairs with curr max key and add them to the new vec
//        for (auto &item: jc->thread_to_vec) {
//            auto curr_vec = item.second;
//            if (curr_vec->empty()) {
//                continue;
//            }
//            if (!curr_vec->empty() && pairs_shuffled < num_of_pairs &&
//                    (!(*curr_max_key < *(curr_vec->back().first)) && !(*(curr_vec->back().first) < *curr_max_key))) {
//                new_vector->push_back(curr_vec->back());
//                curr_vec->pop_back();
//                pairs_shuffled++;
//                (jc->atomic_counter)++;
//                jc->atomic_percentage++;
//            }
//        }
//        // add the new vector to the queue vector
//        if (!new_vector->empty()) {
//            jc->queue_vec.push_back(new_vector);
//        }
//    }
//    //init atomic counter for reduce phase
//    init_atomic_var(REDUCE_STAGE, jc, jc->queue_vec.size());
//
//}

void reducePhase(JobContext *jc) {
    unsigned long queue_size = jc->queue_vec.size();
    bool keepOnMapping = true;
    while (keepOnMapping) {
        lock(&jc->mutex_reduce);
        uint64_t old_value = (jc->atomic_counter)++;
        unlock(&jc->mutex_reduce);
        unsigned long index = get_index(old_value);
        if (index < queue_size) {
//            cout << "I'm thread " << pthread_self() % 1000 << " in Reduce" << endl;
//            cout << "index = " << index << endl;
            auto curr_vec = jc->queue_vec[queue_size - index - 1];
            jc->client.reduce(curr_vec, jc);
            jc->atomic_percentage++;
            queue_size = jc->queue_vec.size();
        } else {
            keepOnMapping = false;
            (jc->atomic_counter)--;
        }
    }
}

void *threadJob(void *arg) {

    auto *jc = (JobContext *) arg;
    lock(&jc->mutex_begin);
    // if this is the first thread, save its ID
    if (!jc->first_thread_created) {
        jc->first_thread = pthread_self();
        jc->first_thread_created = true;
    }
    // create an intermediate vector for the threads
    jc->thread_to_vec.insert(thread_to_vecPair(pthread_self(), new IntermediateVec()));
    unlock(&jc->mutex_begin);
    mapPhase(jc);

    sortPhase(jc);

    jc->barrier->barrier();

//  lock (&print_mutex);
////  cout << "I'm thread "<<pthread_self() % 1000 <<endl;
//  unlock (&print_mutex);
    if (pthread_self() == jc->first_thread) {
        shufflePhase(jc);
    }

//    while (1) {}

    jc->barrier->barrier();

    reducePhase(jc);

    return nullptr;
}

void emit2(K2 *key, V2 *value, void *context) {
    auto jc = (JobContext *) context;
    jc->thread_to_vec[pthread_self()]->push_back(IntermediatePair(key, value));
}

void emit3(K3 *key, V3 *value, void *context) {
    auto jc = (JobContext *) context;
    lock(&jc->mutex_output_vec);
    jc->output_vec.push_back(OutputPair(key, value));
    unlock(&jc->mutex_output_vec);
}

JobHandle startMapReduceJob(const MapReduceClient &client,
                            const InputVec &inputVec, OutputVec &outputVec,
                            int multiThreadLevel) {

    auto thread_to_vec = new map<pthread_t, IntermediateVec *>;
    auto queue_vec = new vector<IntermediateVec *>;

    // init all JobContext elements
    auto *job_context = new JobContext{
            .num_of_threads = multiThreadLevel,
            .threads = new pthread_t[multiThreadLevel],
            .first_thread_created = false,
            .thread_to_vec = *thread_to_vec,
            .queue_vec = *queue_vec,
            .input_vec = inputVec,
            .client = client,
            .output_vec = outputVec,
            .barrier = new Barrier(multiThreadLevel),
            .atomic_counter{0},
            .atomic_percentage{0},
            .mutex_output_vec = PTHREAD_MUTEX_INITIALIZER,
            .mutex_map = PTHREAD_MUTEX_INITIALIZER,
            .mutex_reduce = PTHREAD_MUTEX_INITIALIZER,
//            .mutex_wait_for_job = PTHREAD_MUTEX_INITIALIZER,
//            .mutex_get_state = PTHREAD_MUTEX_INITIALIZER,
            .mutex_begin = PTHREAD_MUTEX_INITIALIZER,
            .waiting = false
    };

    //init atomic counter for map phase
    init_atomic_var(MAP_STAGE, job_context, job_context->input_vec.size());

    // create threads
    for (int i = 0; i < multiThreadLevel; i++) {
        if (pthread_create(&(job_context->threads[i]), nullptr, threadJob, job_context)
            != 0) {
            handleError("error with creating thread");
        }
    }
//  pthread_mutex_destroy (&job_context->mutex_output_vec);
    return static_cast<JobHandle>(job_context);
}

void waitForJob(JobHandle job) {
    auto jc = (JobContext *) job;
    if (!jc->waiting) {
        for (int i = 0; i < jc->num_of_threads; ++i) {
            if (pthread_join(jc->threads[i], nullptr) != 0) {
                handleError("err in waitForJob");
            }
        }
        jc->waiting = true;
    }
}

void getJobState(JobHandle job, JobState *state) {
    auto jc = (JobContext *) job;
    unsigned long counter = jc->atomic_counter.load();
    unsigned long stage = get_stage(counter);
    unsigned long total_size = get_size(counter);
    uint64_t done = jc->atomic_percentage;
    state->stage = (stage_t) (stage);
    float percentage = ((float) done / (float) total_size) * 100;
    state->percentage = percentage;
}

void closeJobHandle(JobHandle job) {
    waitForJob(job);
    auto jc = (JobContext *) job;

    delete jc->threads;
    delete jc->barrier;
    for (auto &pair: jc->thread_to_vec) {
        delete pair.second;
    }
    delete &jc->thread_to_vec;
    delete jc;
}




