#include <defs.h>
#include <list.h>
#include <sync.h>
#include <wait.h>
#include <proc.h>
#include "wait.h"

void
wait_init(wait_t *wait, struct proc_struct *proc) {
    wait->proc = proc;
    wait->wakeup_flags = WT_INTERRUPTED;
    list_init(&(wait->wait_link));
}

void
wait_queue_init(wait_queue_t *queue) {
    list_init(&(queue->wait_head));
    initlock(&queue->lock, "wait_queue");
}


//This method is not atmoic!
void
wait_queue_add(wait_queue_t *queue, wait_t *wait) {
    assert(holding(&queue->lock));
    assert(list_empty(&(wait->wait_link)) && wait->proc != NULL);
    wait->wait_queue = queue;
    list_add_before(&(queue->wait_head), &(wait->wait_link));
}

//This method is not atmoic!
void
wait_queue_del(wait_queue_t *queue, wait_t *wait) {
    assert(holding(&queue->lock));
    assert(!list_empty(&(wait->wait_link)) && wait->wait_queue == queue);
    list_del_init(&(wait->wait_link));
}

//This method is not atomic!
wait_t *
wait_queue_next(wait_queue_t *queue, wait_t *wait) {
    assert(holding(&queue->lock));
    assert(!list_empty(&(wait->wait_link)) && wait->wait_queue == queue);
    list_entry_t *le = list_next(&(wait->wait_link));
    if (le != &(queue->wait_head)) {
        return le2wait(le, wait_link);
    }
    return NULL;
}

//This method is not atomic!
wait_t *
wait_queue_prev(wait_queue_t *queue, wait_t *wait) {
    assert(holding(&queue->lock));
    assert(!list_empty(&(wait->wait_link)) && wait->wait_queue == queue);
    list_entry_t *le = list_prev(&(wait->wait_link));
    if (le != &(queue->wait_head)) {
        return le2wait(le, wait_link);
    }
    return NULL;
}

//This method is not atomic!
wait_t *
wait_queue_first(wait_queue_t *queue) {
    assert(holding(&queue->lock));
    list_entry_t *le = list_next(&(queue->wait_head));
    if (le != &(queue->wait_head)) {
        return le2wait(le, wait_link);
    }
    return NULL;
}

//This method is not atomic!
wait_t *
wait_queue_last(wait_queue_t *queue) {
    assert(holding(&queue->lock));
    list_entry_t *le = list_prev(&(queue->wait_head));
    if (le != &(queue->wait_head)) {
        return le2wait(le, wait_link);
    }
    return NULL;
}

//This method is not atomic!
bool
wait_queue_empty(wait_queue_t *queue) {
    assert(holding(&queue->lock));
    return list_empty(&(queue->wait_head));
}

//TODO:Because of the existense of such test, it seems a single wait needs to be locked, too. So complicated...
//Consider such operation: a wait is moved from a queue to another, and another cpu is testing wait_in_queue,
//lock any queue would not be enough. Let me think about it later.
bool
wait_in_queue(wait_t *wait) {
    return !list_empty(&(wait->wait_link));
}

//This method is not atomic!
void
wakeup_wait(wait_queue_t *queue, wait_t *wait, uint32_t wakeup_flags, bool del) {
    assert(holding(&queue->lock));
    if (del) {
        wait_queue_del(queue, wait);
    }
    wait->wakeup_flags = wakeup_flags;
    lock_schtable();
    wakeup_proc(wait->proc);
    release_schtable();
}

//This method is not atomic!
void
wakeup_first(wait_queue_t *queue, uint32_t wakeup_flags, bool del) {
    assert(holding(&queue->lock));
    wait_t *wait;
    if ((wait = wait_queue_first(queue)) != NULL) {
        wakeup_wait(queue, wait, wakeup_flags, del);
    }
}

//This method is not atomic!
void
wakeup_queue(wait_queue_t *queue, uint32_t wakeup_flags, bool del) {
    assert(holding(&queue->lock));
    wait_t *wait;
    if ((wait = wait_queue_first(queue)) != NULL) {
        if (del) {
            do {
                wakeup_wait(queue, wait, wakeup_flags, 1);
            } while ((wait = wait_queue_first(queue)) != NULL);
        }
        else {
            do {
                wakeup_wait(queue, wait, wakeup_flags, 0);
            } while ((wait = wait_queue_next(queue, wait)) != NULL);
        }
    }
}

//This method is  not atomic!
void
wait_current_set(wait_queue_t *queue, wait_t *wait, uint32_t wait_state) {
    assert(current != NULL);
    assert(holding(&queue->lock));
    wait_init(wait, current);
    current->state = PROC_SLEEPING;
    current->wait_state = wait_state;
    wait_queue_add(queue, wait);
}

void release_wait_table(wait_queue_t *queue) {
    release(&queue->lock);
}

void lock_wait_table(wait_queue_t *queue) {
    acquire(&queue->lock);
}
