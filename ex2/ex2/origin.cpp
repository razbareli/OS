#include <queue>
#include <map>
#include <algorithm>
#include <memory>
#include <string>
#include <iostream>
#include <csignal>           /* Definition of SIG_* constants */
#include <armadillo>
#include <csetjmp>
#include <set>
#include <deque>
#include "uthreads.h"

#define SECOND 1000000

using std::string;

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address (address_t addr)
{
  address_t ret;
  asm volatile("xor    %%fs:0x30,%0\n"
               "rol    $0x11,%0\n"
  : "=g" (ret)
  : "0" (addr));
  return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}
#define SECOND 1000000

#endif

std::set<int> available_ids;
std::vector<int> sleepingThreads;

int total_quantums = 0;
sigset_t masked;

typedef void (*thread_entry_point) (void);

enum State {
  READY, RUNNING, BLOCKED
};

class Thread {

 private:
  int ID;
  thread_entry_point tep;
  int quantnums = 0;
  sigjmp_buf env{};
  char *stack;

 public:
  State state;
  int sleepTime = 0;

  Thread (int ID, thread_entry_point tep) : ID (ID), state (READY), tep (tep)
  {
    if (ID != 0)
      {
        // setup_thread
        stack = new char[STACK_SIZE];
        address_t sp, pc;
        sp = (address_t) stack + STACK_SIZE - sizeof (address_t);
        pc = (address_t) tep;
        sigsetjmp(env, 1);
        (env->__jmpbuf)[JB_SP] = translate_address (sp);
        (env->__jmpbuf)[JB_PC] = translate_address (pc);
        sigemptyset (&env->__saved_mask);
      }

  }

  void increase_quantum ()
  { quantnums++; }

  sigjmp_buf &get_env ()
  { return env; }

  int get_quantnums () const
  { return quantnums; }

};

typedef std::shared_ptr<Thread> smart_thread_ptr;

int quantum;
int running_thread_id;

std::deque<int> ready_deque;
std::map<int, smart_thread_ptr> threads;

struct sigaction sa = {nullptr};
struct itimerval timer;

/**
 * @brief sets a new ID to a new thread
 * @return an id integer
*/
int generate_id ()
{
  int id = *std::min_element (available_ids.begin (), available_ids.end ());
  available_ids.erase (id);
  return id;
}

void handle_err (const string &msg)
{
  std::cerr << "system error: " << msg;
  exit (1);
}

void real_block ()
{
  sigprocmask (SIG_BLOCK, &masked, nullptr);
}

void real_unblock ()
{
  sigprocmask (SIG_UNBLOCK, &masked, nullptr);
}

void handle_sleeping ()
{
  for (auto thread: sleepingThreads)
    {
      threads[thread]->sleepTime--;
      if (threads[thread]->sleepTime == 0)
        {
          uthread_resume (thread);
        }
    }
}

void set_timer_time(){
  timer.it_value.tv_sec = quantum / 1000000;
  timer.it_value.tv_usec = quantum % 1000000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
}

void scheduler (int sig)
{
  // handle_sleeping();
  real_block ();
  std::cout<<"reached tick"<<std::endl;

  if (threads[running_thread_id]->state == RUNNING)
    {
      threads[running_thread_id]->state = READY;
      ready_deque.push_back (running_thread_id);
    }
  if (sigsetjmp(threads[running_thread_id]->get_env (), 1) == 1)
    {
      return;
    }
  running_thread_id = ready_deque.front ();
  ready_deque.pop_front ();
  threads[running_thread_id]->state = RUNNING;
  threads[running_thread_id]->increase_quantum ();
  total_quantums++;
  real_unblock ();

//  timer.it_value.tv_sec = quantum / 1000000;
//  timer.it_value.tv_usec = quantum % 1000000;
//  timer.it_interval.tv_sec = 0;
//  timer.it_interval.tv_usec = 0;
  set_timer_time();
  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
    {
      handle_err ("err 2");
    }

  siglongjmp (threads[running_thread_id]->get_env (), 1);

}

/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init (int quantum_usecs)
{
  if (quantum_usecs <= 0)
    {
      return -1;
    }
  quantum = quantum_usecs;

  for (int i = 1; i < MAX_THREAD_NUM; ++i)
    {
      available_ids.insert (i);
    }

  sa.sa_handler = &scheduler;
  set_timer_time();
  sigemptyset (&masked);
  sigaddset (&masked, SIGVTALRM);
  sa.sa_flags = 0;

  if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
    {
      handle_err ("sigaction err");
    }

  Thread *main_thread = new Thread (0, nullptr);
  smart_thread_ptr newThreadP (main_thread);
  threads[0] = newThreadP;
  running_thread_id = 0;
  newThreadP->state = RUNNING;

  if (sigsetjmp(newThreadP->get_env (), 1) != 0)
    {
      handle_err ("err sigsetjmp");
    }
  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
    {
      handle_err ("err setitimer");
    }

  return 0;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn (thread_entry_point entry_point)
{
  real_block ();
  if (threads.size () == MAX_THREAD_NUM)
    {
      real_unblock ();
      return -1;
    }
  int id = generate_id ();

  Thread *thread = new Thread (id, entry_point);
  smart_thread_ptr newThreadP (thread);
  threads[id] = newThreadP;
  ready_deque.push_back (id);

  real_unblock ();
  return id;
}

void removeFromReady (int tid)
{
  std::deque<int>::iterator itr;
  itr = find (ready_deque.begin (), ready_deque.end (), tid);
  if (itr != ready_deque.end ())
    {
//      auto idx = std::distance(ready_deque.begin(), itr);
      ready_deque.erase (itr);
    }
  else
    {
      handle_err ("not in ready");
    }
}

void validateTid (int tid, bool careAboutMain = false)
{
  if ((careAboutMain && threads.count (tid) == 0) || tid == 0)
    {
      real_unblock ();
      handle_err ("error with id");
    }
}

/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate (int tid)
{
  real_block ();

  if (tid == 0)
    {
      real_unblock ();
      exit (0);
    } // todo check if legal, valgrind

  if (threads.count (tid) == 0)
    {
      real_unblock ();
      handle_err ("no thread with ID tid exists");
    }

  if (threads[tid]->state == READY)
    {
      removeFromReady (tid);
    }

  if (threads[tid]->state == RUNNING)
    {
//      real_unblock ();
      scheduler (0);
      removeFromReady (tid);
    }

  threads.erase (tid);
  available_ids.insert (tid);

  real_unblock ();
  return 0;

}

/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block (int tid)
{
  real_block ();

  validateTid (tid, true);

  if (threads[tid]->state == READY)
    {
      threads[tid]->state = BLOCKED;
      removeFromReady (tid);
    }
  else if (threads[tid]->state == RUNNING)
    {
      real_unblock ();
      scheduler (0);
      removeFromReady (tid);
    }

  real_unblock ();
  return 0;
}

/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume (int tid)
{
  real_block ();
  validateTid (tid);

  if (threads[tid]->state == BLOCKED)
    {
      if (threads.size () == MAX_THREAD_NUM)
        {
          real_unblock ();
          return -1;
        }
      threads[tid]->state = READY;
      ready_deque.push_back (tid);
    }

  real_unblock ();
  return 0;
}

/**
 * @brief Blocks the RUNNING thread for num_quantums quantnums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * The number of quantnums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep (int num_quantums)
{
  real_block ();

  threads[running_thread_id]->sleepTime = num_quantums;
  threads[running_thread_id]->state = BLOCKED;
  sleepingThreads.push_back (running_thread_id);
  scheduler (0);

  real_unblock ();
  return 0;
}

/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid ()
{
  return running_thread_id;
}

/**
 * @brief Returns the total number of quantnums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantnums.
*/
int uthread_get_total_quantums ()
{
  return total_quantums;
}

/**
 * @brief Returns the number of quantnums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantnums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums (int tid)
{
  real_block ();
  if (threads.count (tid) == 0)
    {
      real_unblock ();
      handle_err ("error with id");
    }
  int num = threads[tid]->get_quantnums ();
  real_unblock ();
  return num;
}

int uthread_mutex_lock ()
{
  return 0;
}

int uthread_mutex_unlock ()
{
  return 0;
}