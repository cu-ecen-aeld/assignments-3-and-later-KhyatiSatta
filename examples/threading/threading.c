#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter

    // Cast the void * parameter to the thread data structure
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // For return status for each function
    int ret_status = 0;

    // Wait for given milliseconds before acquiring the mutex
    ret_status = usleep((thread_func_args->wait_to_obtain_ms) * 1000);

    // Error check: If the usleep function was not successful
    if(ret_status == -1){
        ERROR_LOG("usleep function (before acquiring the mutex lock) failed with the error code: %d\n", errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    else{
        DEBUG_LOG("usleep function (before acquiring the mutex lock) successful\n");
        thread_func_args->thread_complete_success = true;
    }

    // Acquire the mutex lock
    ret_status = pthread_mutex_lock(thread_func_args->mutex);
    
    // Error check: If there was an issue with acquiring the lock
    if (ret_status != 0){
        ERROR_LOG("Error acquiring mutex with the error code: %d\n" , errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    else{
        DEBUG_LOG("Mutex locking successful\n");
        thread_func_args->thread_complete_success = true;
    }

    // Wait for given milliseconds before release the mutex
    ret_status = usleep((thread_func_args->wait_to_release_ms) * 1000);

    // Error check: If the usleep function was not successful
    if(ret_status == -1){
        ERROR_LOG("usleep function (after acquiring the mutex lock)failed with the error code: %d\n", errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    else{
        DEBUG_LOG("usleep function (after acquiring the mutex lock) successful\n");
        thread_func_args->thread_complete_success = true;
    }

    // Release the mutex lock
    ret_status = pthread_mutex_unlock(thread_func_args->mutex);

    // Error check: If there was an issue with releasing the lock
    if (ret_status != 0){
        ERROR_LOG("Error releasing mutex with the error code: %d\n" , errno);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    else{
        DEBUG_LOG("Mutex unlocking successful\n");
        thread_func_args->thread_complete_success = true;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    /**
* Start a thread which sleeps @param wait_to_obtain_ms number of milliseconds, then obtains the
* mutex in @param mutex, then holds for @param wait_to_release_ms milliseconds, then releases.
* The start_thread_obtaining_mutex function should only start the thread and should not block
* for the thread to complete.
* The start_thread_obtaining_mutex function should use dynamic memory allocation for thread_data
* structure passed into the thread.  The number of threads active should be limited only by the
* amount of available memory.
* The thread started should return a pointer to the thread_data structure when it exits, which can be used
* to free memory as well as to check thread_complete_success for successful exit.
* If a thread was started succesfully @param thread should be filled with the pthread_create thread ID
* coresponding to the thread which was started.
* @return true if the thread could be started, false if a failure occurred.
*/
    // Allocate space for the data structure dynamically
    struct thread_data *thread_params = (struct thread_data *)malloc(sizeof(struct thread_data));

    // Error check: If malloc failed
    if(thread_params == NULL){
        ERROR_LOG("Malloc for the thread data structure failed\n");
        return false;
    }

    // Store the parameters in the thread data structure
    thread_params->thread = thread;
    thread_params->mutex = mutex;
    thread_params->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_params->wait_to_release_ms = wait_to_release_ms;

    //Create the thread
    int ret_status = pthread_create(thread , NULL , threadfunc , thread_params);
    
    // Error check: Check if the thread was created/started
    if(ret_status != 0){
        ERROR_LOG("Unable to create/start the thread\n");
        return false;
    }
    else{
        DEBUG_LOG("Thread created/started\n");
        return true;
    }
}

