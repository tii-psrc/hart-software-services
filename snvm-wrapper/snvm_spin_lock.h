#ifndef SNVM_SPIN_LOCK_H
#define SNVM_SPIN_LOCK_H

typedef volatile int snvm_spinlock_t;

static inline void snvm_spin_lock(snvm_spinlock_t *lock)
{
  while (__sync_lock_test_and_set(lock, 1))
    ;
}

static inline void snvm_spin_unlock(snvm_spinlock_t *lock)
{
  __sync_lock_release(lock);
}

#endif
