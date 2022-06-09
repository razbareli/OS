#include "uthreads.h"
#include <signal.h>
#include <sys/time.h>
#include <iostream>
#include <map>
#include <setjmp.h>
#include <deque>
#include <algorithm>
#include <queue>
#include <vector>

#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

/* variable for masking */
sigset_t masked;

/* the quantum value in this program */
int quantum_val;

/*current running thread*/
int running_thread_id;

/*total quantums passed*/
int total_quantums = 1;

/* typedef for function pointer */
typedef void (*thread_entry_point) (void);

/*error types*/
enum Error {
  LIB, SYS
};

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

#endif

class Thread {
 private:
  int id;
  int quantums = 0;
  bool blocked = false;
 public:

  /*ready and running*/
  bool ready = false;
  bool running = false;
  int sleep = 0;

  /*constructor*/
  explicit Thread (int id) : id (id)
  {};

  /*getters*/
  int get_quantums () const
  { return quantums; }
  int get_sleep () const
  { return sleep; }
  bool is_blocked () const
  { return blocked; }

  /*setters*/
  void inc_quantums ()
  { quantums++; }
  void dec_sleep ()
  { sleep--; }
  void block ()
  { blocked = true; }
  void unblock ()
  { blocked = false; }
};
/* min heap of available ID's to use as names for threads*/
std::priority_queue<int, std::vector<int>, std::greater<int> > available_ids;

/* double sided queue for ready threads */
std::deque<int> ready_deque;

/* list of current threads
 * the main thread is in index 0 */
Thread *threads[MAX_THREAD_NUM];

/* environments of threads*/
sigjmp_buf env[MAX_THREAD_NUM];

/* arrays of stacks for the existing threads */
char *stack[MAX_THREAD_NUM];

/**
 * masks real signals
 */
void real_block ()
{
  sigprocmask (SIG_BLOCK, &masked, nullptr);
}

/**
 * unblocks real signals
 */
void real_unblock ()
{
  sigprocmask (SIG_UNBLOCK, &masked, nullptr);
}

/**
 * handles error
 * @param msg what message to print
 */
void handle_err (const std::string &msg, Error type)
{
  if (type == SYS)
    {
      std::cerr << "system error: " << msg << std::endl;
      exit (1);
    }
  //if (type == LIB):
  std::cerr << "thread library error: " << msg << std::endl;
}


/* deletes a thread with all its mamory allocations */
void delete_thread (int id)
{
  real_block ();
  delete threads[id];
  delete [] stack[id];
  threads[id] = nullptr;
  stack[id] = nullptr;
  available_ids.push (id);
  real_unblock ();
}

void free_memory ()
{
  for (int i = 1; i < MAX_THREAD_NUM; i++)
  {
    delete_thread (i);
    if (stack[i] != nullptr)
    {
      delete[] stack[i];
    }
  }
}


/**
 * @brief sets a new ID to a new thread
 * @return an id integer
*/
int generate_id ()
{
  if (available_ids.empty ())
    {
      handle_err ("you've reached max num of threads", LIB);
      return -1;
    }
  int new_id = available_ids.top ();
  available_ids.pop ();
  return new_id;
}

int remove_from_ready (int tid)
{
  std::deque<int>::iterator itr;
  itr = find (ready_deque.begin (), ready_deque.end (), tid);
  if (itr != ready_deque.end ())
    {
      ready_deque.erase (itr);
      threads[tid]->ready = false;
    }
  else
    {
      handle_err ("not in ready", LIB);
      return -1;
    }
  return 0;
}



struct sigaction sa = {nullptr};
struct itimerval timer;

void return_to_ready (int id)
{
  ready_deque.push_back (id);
  threads[id]->ready = true;
}

/**
 * handles the sleeping threads
 */
void handle_sleeping ()
{
  for (int i = 1; i < MAX_THREAD_NUM; i++)
    {
      if (threads[i] != nullptr)
        {
          if (threads[i]->get_sleep () != 0)
            {
              threads[i]->dec_sleep ();
              if (threads[i]->get_sleep () == 0 && !threads[i]->is_blocked ())
                {
                  return_to_ready (i);
                }
            }
        }
    }
}

//void make_switch ()
//{
//  threads[running_thread_id]->running = false;
//  running_thread_id = ready_deque.front ();
//  threads[running_thread_id]->running = true;
//  threads[running_thread_id]->ready = false;
//  ready_deque.pop_front ();
//  threads[running_thread_id]->inc_quantums ();
//  handle_sleeping();
//
//  //set time
//  timer.it_value.tv_sec = quantum_val / 1000000;
//  timer.it_value.tv_usec = quantum_val % 1000000;
//  timer.it_interval.tv_sec = 0;
//  timer.it_interval.tv_usec = 0;
//  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
//    {
//      handle_err ("err ITIMER_VIRTUAL", SYS);
//    }
//  total_quantums++;
//  siglongjmp (env[running_thread_id], 1);
//}

/* called only by the system - means a quantum has passed */
void scheduler (int sig)
{
  real_block ();

  int has_returned = 0;
  has_returned = sigsetjmp(env[running_thread_id], 1);

  if (!has_returned)
    {
      if (!threads[running_thread_id]->is_blocked ()
          && threads[running_thread_id]->sleep <= 0)
        {
          ready_deque.push_back (running_thread_id);
          threads[running_thread_id]->ready = true;
        }
      threads[running_thread_id]->running = false;
      running_thread_id = ready_deque.front ();
      threads[running_thread_id]->running = true;
      threads[running_thread_id]->ready = false;
      ready_deque.pop_front ();
      total_quantums++;
      threads[running_thread_id]->inc_quantums ();
      handle_sleeping();

      timer.it_value.tv_sec = quantum_val / 1000000;
      timer.it_value.tv_usec = quantum_val % 1000000;
      timer.it_interval.tv_sec = 0;
      timer.it_interval.tv_usec = 0;

      if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
        {
          handle_err ("err ITIMER_VIRTUAL", SYS);
        }
      siglongjmp (env[running_thread_id], 1);

    }
  real_unblock ();
}

// in case a thread terminates itself
void terminated_switch ()
{
  int terminated_thread = running_thread_id;

  threads[running_thread_id]->running = false;
  running_thread_id = ready_deque.front ();
  threads[running_thread_id]->running = true;
  threads[running_thread_id]->ready = false;
  ready_deque.pop_front ();
  total_quantums++;
  threads[running_thread_id]->inc_quantums ();
  handle_sleeping();

  timer.it_value.tv_sec = quantum_val / 1000000;
  timer.it_value.tv_usec = quantum_val % 1000000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  delete_thread (terminated_thread);

  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
  {
    handle_err ("err ITIMER_VIRTUAL", SYS);
  }
  siglongjmp (env[running_thread_id], 1);


//  make_switch ();

}

// in case a thread blocked itself
//void sleep_or_blocked_switch ()
//{
//  if (sigsetjmp(env[running_thread_id], 1) == 1)
//    {
//      real_unblock ();
//      return;
//    }
//  make_switch ();
//}

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

  sigemptyset (&masked);
  sigaddset (&masked, SIGVTALRM);
  real_block ();

  // fill the min heap with id's
  for (int i = 1; i < MAX_THREAD_NUM; ++i)
    {
      available_ids.push (i);
    }
  // check quantum number
  if (quantum_usecs <= 0)
    {
      handle_err ("invalid quantum_usecs", LIB);
      return -1;
    }
  quantum_val = quantum_usecs;

  //set time
  timer.it_value.tv_sec = quantum_val / 1000000;
  timer.it_value.tv_usec = quantum_val % 1000000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
    {
      handle_err ("err setitimer", SYS);
    }

  sa.sa_handler = &scheduler;
  if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
    {
      handle_err ("sigaction err", SYS);
    }

  threads[0] = new Thread (0);
  running_thread_id = 0;
  threads[0]->running = true;

  sigsetjmp(env[0], 1);
  (env[0]->__jmpbuf)[JB_SP] = (long) __builtin_frame_address (0);
  (env[0]->__jmpbuf)[JB_PC] = (long) __builtin_return_address (0);
  sigemptyset (&env[0]->__saved_mask);

  threads[0]->inc_quantums ();
  real_unblock ();
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

  if (entry_point == nullptr){
      handle_err ("entry point is null", LIB);
      real_unblock();
      return -1;
  }

  int new_id = generate_id ();
  if (new_id == -1)
    {
      real_unblock ();
      return -1;
    }
  stack[new_id] = new char[STACK_SIZE];
  threads[new_id] = new Thread (new_id);

  address_t sp, pc;
  sp = (address_t) stack[new_id] + STACK_SIZE - sizeof (address_t);
  pc = (address_t) entry_point;
  sigsetjmp(env[new_id], 1);
  (env[new_id]->__jmpbuf)[JB_SP] = (long) translate_address (sp);
  (env[new_id]->__jmpbuf)[JB_PC] = (long) translate_address (pc);
  sigemptyset (&env[new_id]->__saved_mask);

  ready_deque.push_back (new_id);
  threads[new_id]->ready = true;
  real_unblock ();
  return new_id;
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
      free_memory ();
      real_unblock ();
      exit (0);
    }
    else if (tid >= MAX_THREAD_NUM || tid < 0 || threads[tid] == nullptr)
    {
      handle_err ("no such tid in terminate func", LIB);
      real_unblock ();
      return -1;
    }
  else if (tid == running_thread_id)
    {

      terminated_switch ();
      //threads[tid]->block ();
//      scheduler (0);
//      delete_thread (tid);
    }
  else if (!threads[tid]->is_blocked())
    {
      if (threads[tid]->get_sleep() <= 0){
        remove_from_ready (tid);
      }
      delete_thread (tid);
    }
  else if (threads[tid]->is_blocked ())
    {
      delete_thread (tid);
    }
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

  if (tid >= MAX_THREAD_NUM || tid < 0 || threads[tid] == nullptr)
    {
      handle_err ("no such tid in block func", LIB);
      real_unblock ();
      return -1;
    }
  else if (tid == 0)
    {
      handle_err ("trying block the main thread", LIB);
      real_unblock ();
      return -1;
    }
  else if (tid == running_thread_id)
    {
      threads[tid]->block ();
      scheduler (0);
    }
  else if (!threads[tid]->is_blocked ())
    {
      threads[tid]->block ();
      // remove from ready queue
      if (threads[tid]->get_sleep() <= 0){
        remove_from_ready (tid);
      }

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

  if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr
      || (stack[tid] == nullptr && (tid != 0)))
    {
      handle_err ("no such tid in resume func", LIB);
      real_unblock ();
      return -1;
    }

  if (threads[tid]->get_sleep () <= 0 &&
      !threads[tid]->ready &&
      !threads[tid]->running &&
      threads[tid]->is_blocked ())
    {
      threads[tid]->unblock ();
      ready_deque.push_back (tid);
    }
  else if (threads[tid]->get_sleep () > 0)
    {
      threads[tid]->unblock ();
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

  if (running_thread_id == 0)
    {
      handle_err ("main thread can't sleep", LIB);
      real_unblock ();
      return -1;
    }
  else if (num_quantums > 0)
    {
//      sleep_or_blocked_switch ();
      threads[running_thread_id]->sleep = num_quantums;
      scheduler (0);
    }
  real_unblock ();
  return 0;
}

/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid ()
{ return running_thread_id; }

/**
 * @brief Returns the total number of quantnums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantnums.
*/
int uthread_get_total_quantums ()
{ return total_quantums; }

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

  if (tid >= MAX_THREAD_NUM || tid < 0 || threads[tid] == nullptr)
    {
      handle_err ("no such tid in get_quantum func", LIB);
      real_unblock ();
      return -1;
    }
  int num = threads[tid]->get_quantums ();
  real_unblock ();
  return num;
}




