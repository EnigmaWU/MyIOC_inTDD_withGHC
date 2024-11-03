# _IOC_EvtDescQueue.c

This file implements the event descriptor queue, which is used to safely manage event enqueue and dequeue operations in a multithreaded environment.

## Function Overview

- **Initialize the Queue**: The `_IOC_EvtDescQueue_initOne` function initializes the event descriptor queue, sets up the mutex lock, and clears the queue.
- **Destroy the Queue**: The `_IOC_EvtDescQueue_deinitOne` function destroys the queue, releases the mutex lock, and performs related state checks.
- **Check if the Queue is Empty**: The `_IOC_EvtDescQueue_isEmpty` function checks whether the event queue is empty and returns the corresponding boolean result.
- **Enqueue Event**: The `_IOC_EvtDescQueue_enqueueElementLast` function enqueues new event descriptors, ensures the queue is not full, and updates queue pointers.
- **Dequeue Event**: The `_IOC_EvtDescQueue_dequeueElementFirst` function dequeues event descriptors from the queue, ensures the queue is not empty, and updates queue pointers.

## Notes

- **Thread Safety**: All operations use a `pthread_mutex_t` mutex named `Mutex` to ensure thread safety.
- **Queue Capacity Limit**: The maximum capacity of the queue is defined by `_CONLES_EVENT_MAX_QUEUING_EVTDESC`. Be aware that enqueue operations may fail if the queue is full.
- **Queue Pointer Management**: `QueuedEvtNum` and `ProcedEvtNum` are used to track the number of enqueued and processed events, managing the queue as a circular buffer.
- **Error Checking**: Assertions using `_IOC_LogAssert` are employed to ensure the correct state of the queue.

## Usage

1. Call `_IOC_EvtDescQueue_initOne` to initialize the event queue.
2. Use `_IOC_EvtDescQueue_enqueueElementLast` and `_IOC_EvtDescQueue_dequeueElementFirst` for enqueue and dequeue operations.
3. Use `_IOC_EvtDescQueue_isEmpty` to check if the queue is empty.
4. When no longer needed, call `_IOC_EvtDescQueue_deinitOne` to destroy the queue.

## TODO

- Implement error handling for mutex operations (e.g., check return values of `pthread_mutex_lock` and `pthread_mutex_unlock`).
- Add detailed logging for enqueue and dequeue actions to assist in debugging.
- Optimize the queue management logic to improve performance under high concurrency.

