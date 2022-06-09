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
    stage_t stage;

    //synchronization tools
    Barrier *barrier;
    atomic<uint64_t> atomic_counter;
    atomic<uint64_t> atomic_percentage;

    pthread_mutex_t *mutex_output_vec;
//    pthread_mutex_t mutex_map;
//    pthread_mutex_t mutex_reduce;
    pthread_mutex_t *mutex_wait_for_job;
    pthread_mutex_t *mutex_get_state;
//    pthread_mutex_t mutex_update_done_map;
//    pthread_mutex_t mutex_update_done_shuffle;
    pthread_mutex_t *mutex_begin;

    // status of the job
    bool waiting;

} JobContext;

void lock(pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
}

void unlock(pthread_mutex_t *mutex) {
    pthread_mutex_unlock(mutex);
}

void init_atomic_var(stage_t stage, JobContext *jc, unsigned long total_size) {
//    lock(jc->mutex_get_state);
    uint64_t casted_stage = stage;
    casted_stage = casted_stage << 62;
    uint64_t num_of_pairs = total_size << 31;
    jc->atomic_counter = num_of_pairs | casted_stage;
    jc->atomic_percentage = 0;
//    unlock(jc->mutex_get_state);
}

void mapPhase(JobContext *jc) {

    size_t input_vec_size = jc->input_vec.size();
    bool keepOnMapping = true;
    while (keepOnMapping) {
//        lock(&jc->mutex_map);
        uint64_t old_value = (jc->atomic_counter)++;
//        unlock(&jc->mutex_map);
        unsigned long index = get_index(old_value);
        if (index < input_vec_size ) {
            InputPair pair = jc->input_vec[get_index(index)];
            jc->client.map(pair.first, pair.second, jc);
            jc->atomic_percentage++;

        } else {
            keepOnMapping = false;
            (jc->atomic_counter)--;

        }

    }


}

bool comparator(IntermediatePair &a, IntermediatePair &b) {
    return ((*a.first) < (*b.first));
}

void sortPhase(JobContext *jc) {
    auto *vev_to_sort = jc->thread_to_vec[pthread_self()];
    sort(vev_to_sort->begin(), vev_to_sort->end(), comparator);
}

K2 *findMax(JobContext *jc) {
    K2 *cur_max = nullptr;

    for (auto &item: jc->thread_to_vec) {
        auto curr_vec = item.second;
        if (curr_vec->empty()) {
            continue;
        }
        K2 *max_key_vec = curr_vec->back().first;
        if (cur_max == nullptr || *cur_max < *max_key_vec) {
            cur_max = max_key_vec;
        }
    }

    return cur_max;
}

void shufflePhase(JobContext *jc) {

    unsigned long num_of_pairs = 0;
    for (auto &t: jc->thread_to_vec) {
        num_of_pairs += t.second->size();
    }
    init_atomic_var(SHUFFLE_STAGE, jc, num_of_pairs);
//    cout<<"I'm thread "<<pthread_self() % 1000<<" in shuffle"<<endl;
    unsigned long counter = jc->atomic_counter.load();
    unsigned long stage = get_stage(counter);

    K2 *max = findMax(jc);

    while (max != nullptr && stage == 2) {
        auto new_vector = new IntermediateVec();

        for (auto &item: jc->thread_to_vec) {
            auto curr_vec = item.second;
            while (!curr_vec->empty() && (!(*max < *(curr_vec->back().first))
                                          && !(*(curr_vec->back().first)
                                               < *max))) {
                new_vector->push_back(curr_vec->back());
                curr_vec->pop_back();
//               jc->atomic_percentage++;
//              (jc->atomic_counter)++;
            }
        }

        if (!new_vector->empty()) {
            jc->queue_vec.push_back(new_vector);
            (jc->atomic_percentage) += new_vector->size();
        }

        max = findMax(jc);
    }

    init_atomic_var(REDUCE_STAGE, jc, jc->queue_vec.size());
}

void reducePhase(JobContext *jc) {


    unsigned long queue_size = jc->queue_vec.size();

    bool keepOnMapping = true;

    while (keepOnMapping) {
//        lock(&jc->mutex_reduce);
        uint64_t old_value = (jc->atomic_counter)++;
        unsigned long index = get_index(old_value);
//        unlock(&jc->mutex_reduce);
        if (index < queue_size ) {
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

    lock(jc->mutex_begin); // todo better if i==0
    // if this is the first thread, save its ID
    if (!jc->first_thread_created) {
        jc->first_thread = pthread_self();
        jc->first_thread_created = true;
    }
    // create an intermediate vector for the threads
    jc->thread_to_vec.insert(thread_to_vecPair(pthread_self(), new IntermediateVec()));
    unlock(jc->mutex_begin);

    mapPhase(jc);

    sortPhase(jc);

    jc->barrier->barrier();

    if (pthread_self() == jc->first_thread) {
        shufflePhase(jc);
    }

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
    lock(jc->mutex_output_vec);
    jc->output_vec.push_back(OutputPair(key, value));
    unlock(jc->mutex_output_vec);
}

JobHandle startMapReduceJob(const MapReduceClient &client,
                            const InputVec &inputVec, OutputVec &outputVec,
                            int multiThreadLevel) {

    auto thread_to_vec = new map<pthread_t, IntermediateVec *>;
    auto queue_vec = new vector<IntermediateVec *>;

    auto *mutex_output_vec = new pthread_mutex_t;
    pthread_mutex_init(mutex_output_vec, nullptr);
    auto *mutex_wait_for_job = new pthread_mutex_t;
    pthread_mutex_init(mutex_wait_for_job, nullptr);
    auto *mutex_begin = new pthread_mutex_t;
    pthread_mutex_init(mutex_begin, nullptr);
    auto *mutex_get_state = new pthread_mutex_t;
    pthread_mutex_init(mutex_get_state, nullptr);

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
            .stage = UNDEFINED_STAGE,

            .barrier = new Barrier(multiThreadLevel),
            .atomic_counter{0},
            .atomic_percentage{0},

            .mutex_output_vec = mutex_output_vec,
//            .mutex_map = PTHREAD_MUTEX_INITIALIZER,
//            .mutex_reduce = PTHREAD_MUTEX_INITIALIZER,
.mutex_wait_for_job = mutex_wait_for_job,
.mutex_get_state = mutex_get_state,
//            .mutex_update_done_map =  PTHREAD_MUTEX_INITIALIZER,
//            .mutex_update_done_shuffle = PTHREAD_MUTEX_INITIALIZER,
.mutex_begin = mutex_begin,
            .waiting = false
    };

    //init atomic counter for map phase
    if (job_context->stage == UNDEFINED_STAGE){
        init_atomic_var(MAP_STAGE, job_context, job_context->input_vec.size());
        job_context->stage = MAP_STAGE;
    }

    // create threads
    for (int i = 0; i < multiThreadLevel; i++) {
        if (pthread_create(&(job_context->threads[i]), nullptr, threadJob, job_context)
            != 0) {
            handleError("error with creating thread");
        }
    }


    return static_cast<JobHandle>(job_context);
}

void waitForJob(JobHandle job) {
    auto jc = (JobContext *) job;
    lock(jc->mutex_wait_for_job);
    if (!jc->waiting) {
        for (int i = 0; i < jc->num_of_threads; ++i) {
            if (pthread_join(jc->threads[i], nullptr) != 0) {
                handleError("err in waitForJob");
            }
        }
        jc->waiting = true;
    }
    unlock(jc->mutex_wait_for_job);

}

void getJobState(JobHandle job, JobState *state) {
    auto jc = (JobContext *) job;
//    lock(jc->mutex_get_state);
    unsigned long counter = jc->atomic_counter.load();
    unsigned long stage = get_stage(counter);
    unsigned long total_size = get_size(counter);
    uint64_t done = jc->atomic_percentage;
    state->stage = (stage_t) (stage);
    state->percentage = ((float) done / (float) total_size) * 100;
//    unlock(jc->mutex_get_state);

}

void closeJobHandle(JobHandle job) {
    waitForJob(job);
    auto jc = (JobContext *) job;

    delete jc->barrier;

    for (auto &pair: jc->thread_to_vec) {
        delete pair.second;
    }
    delete &jc->thread_to_vec;

    for (auto &vec: jc->queue_vec) {
        delete vec;
    }
    delete &jc->queue_vec;
    pthread_mutex_destroy(jc->mutex_output_vec);
//    pthread_mutex_destroy(&jc->mutex_map);
//    pthread_mutex_destroy(&jc->mutex_reduce);
    pthread_mutex_destroy(jc->mutex_get_state);
//    pthread_mutex_destroy(&jc->mutex_update_done_map);
//    pthread_mutex_destroy(&jc->mutex_update_done_shuffle);
    pthread_mutex_destroy(jc->mutex_begin);
    pthread_mutex_destroy(jc->mutex_wait_for_job);

    delete jc->mutex_begin;
    delete jc->mutex_wait_for_job;
    delete jc->mutex_output_vec;
    delete jc->mutex_get_state;

    delete[] jc->threads;
    delete jc;
    jc = nullptr;
    job = nullptr;
}




