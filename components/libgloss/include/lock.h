#pragma once


#include <sys/lock.h>

struct __lock {
    void* lock;
};
typedef struct __lock _lock_t;
#define _lock_init(lock) __retarget_lock_init(lock) 
#define _lock_init_recursive(lock) __retarget_lock_init_recursive(lock)
#define _lock_close(lock) __retarget_lock_close(lock)
#define _lock_close_recursive(lock) __retarget_lock_close_recursive(lock)
#define _lock_acquire(lock) __retarget_lock_acquire(lock)
#define _lock_acquire_recursive(lock) __retarget_lock_acquire_recursive(lock)
#define _lock_try_acquire(lock) __retarget_lock_try_acquire(lock)
#define _lock_try_acquire_recursive(lock) __retarget_lock_try_acquire_recursive(lock)
#define _lock_release(lock) __retarget_lock_release(lock)
#define _lock_release_recursive(lock) __retarget_lock_release_recursive(lock)

#define LOCK_INIT_VAL {0}