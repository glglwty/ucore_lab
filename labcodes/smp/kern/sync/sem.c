#include <defs.h>
#include <wait.h>
#include <atomic.h>
#include <kmalloc.h>
#include <sem.h>
#include <proc.h>
#include <sync.h>
#include <assert.h>
#include "sem.h"

void
sem_init(semaphore_t *sem, int value) {
    sem->value = value;
    initlock(&sem->lock, "semaphore");
    wait_queue_init(&(sem->wait_queue));
}

static __noinline void __up(semaphore_t *sem, uint32_t wait_state) {
    bool intr_flag;
    local_intr_save(intr_flag);
    acquire(&sem->lock);
    lock_wait_table(&sem->wait_queue);
    {
        wait_t *wait;
        if ((wait = wait_queue_first(&(sem->wait_queue))) == NULL) {
            sem->value ++;
        }
        else {
            assert(wait->proc->wait_state == wait_state);
            wakeup_wait(&(sem->wait_queue), wait, wait_state, 1);
        }
    }
    release_wait_table(&sem->wait_queue);
    release(&sem->lock);
    local_intr_restore(intr_flag);
}

static __noinline uint32_t __down(semaphore_t *sem, uint32_t wait_state) {
    bool intr_flag;
    local_intr_save(intr_flag);
    acquire(&sem->lock);
    if (sem->value > 0) {
        sem->value --;
        release(&sem->lock);
        local_intr_restore(intr_flag);
        return 0;
    }
    wait_t __wait, *wait = &__wait;
    lock_wait_table(&sem->wait_queue);
    wait_current_set(&(sem->wait_queue), wait, wait_state);
    release_wait_table(&sem->wait_queue);
    release(&sem->lock);
    local_intr_restore(intr_flag);

    schedule();

    local_intr_save(intr_flag);
    acquire(&sem->lock);
    lock_wait_table(&sem->wait_queue);
    wait_current_del(&(sem->wait_queue), wait);
    release_wait_table(&sem->wait_queue);
    release(&sem->lock);
    local_intr_restore(intr_flag);

    if (wait->wakeup_flags != wait_state) {
        return wait->wakeup_flags;
    }
    return 0;
}

void
up(semaphore_t *sem) {
    __up(sem, WT_KSEM);
}

void
down(semaphore_t *sem) {
    uint32_t flags = __down(sem, WT_KSEM);
    assert(flags == 0);
}

bool
try_down(semaphore_t *sem) {
    bool intr_flag, ret = 0;
    local_intr_save(intr_flag);
    acquire(&sem->lock);
    if (sem->value > 0) {
        sem->value --, ret = 1;
    }
    release(&sem->lock);
    local_intr_restore(intr_flag);
    return ret;
}

