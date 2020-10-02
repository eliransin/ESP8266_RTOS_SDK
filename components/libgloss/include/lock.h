#pragma once


#include <stdint.h>
#include <sys/lock.h>

struct __lock {
    intptr_t lock;
};
typedef struct __lock _lock_t;
static inline void _lock_init(_lock_t* lock) {
    _lock_t* lock_temp;
    __retarget_lock_init(&lock_temp);
    *lock = *lock_temp;

}

static inline void _lock_init_recursive(_lock_t* lock) {
    _lock_t* lock_temp;
    __retarget_lock_init_recursive(&lock_temp);
    *lock = *lock_temp;
}

static inline void _lock_close(_lock_t* lock) {
    __retarget_lock_close(lock);
} 
static inline void _lock_close_recursive(_lock_t* lock) {
    __retarget_lock_close_recursive(lock);
}

static inline void _lock_acquire(_lock_t* lock) {
    __retarget_lock_acquire(lock);
}
static inline void _lock_acquire_recursive(_lock_t* lock) {
    __retarget_lock_acquire_recursive(lock);
}
static inline int _lock_try_acquire(_lock_t* lock) {
    return __retarget_lock_try_acquire(lock);
}
static inline int _lock_try_acquire_recursive(_lock_t* lock) {
    return __retarget_lock_try_acquire_recursive(lock);
}

static inline void _lock_release(_lock_t* lock) {
    __retarget_lock_release(lock);
}
static inline void _lock_release_recursive(_lock_t* lock) {
    __retarget_lock_release_recursive(lock);
}

#define LOCK_INIT_VAL {0}