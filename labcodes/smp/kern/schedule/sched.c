#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <stdio.h>
#include <assert.h>
#include <default_sched.h>
#include "assert.h"
#include <spinlock.h>

// the list of timer
static struct timer_table_t {
    struct spinlock lock;
    list_entry_t timer_list;
} timer_table;

static struct sched_class *sched_class;

static struct run_queue *rq;

static inline void
sched_class_enqueue(struct proc_struct *proc) {
    if (proc != idleproc) {
        sched_class->enqueue(rq, proc);
    }
}

static inline void
sched_class_dequeue(struct proc_struct *proc) {
    sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *
sched_class_pick_next(void) {
    return sched_class->pick_next(rq);
}

static void
sched_class_proc_tick(struct proc_struct *proc) {
    if (proc != NULL) {
        if (proc != idleproc) {
            sched_class->proc_tick(rq, proc);
        }
        else {
            proc->need_resched = 1;
        }
    }
}

static struct run_queue __rq;

void
sched_init(void) {
    list_init(&timer_table.timer_list);
    initlock(&timer_table.lock, "timer table");
    sched_class = &default_sched_class;
    initlock(&sched_class->lock, "scheduler");

    rq = &__rq;
    rq->max_time_slice = MAX_TIME_SLICE;
    sched_class->init(rq);

    cprintf("sched class: %s\n", sched_class->name);
}

void
wakeup_proc(struct proc_struct *proc) {
    assert(proc->state != PROC_ZOMBIE);
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        if (proc->state != PROC_RUNNABLE) {
            proc->state = PROC_RUNNABLE;
            proc->wait_state = 0;
            if (proc != current) {
                sched_class_enqueue(proc);
            }
        }
        else {
            warn("wakeup runnable process.\n");
        }
    }
    local_intr_restore(intr_flag);
}

void
schedule(void) {
    bool intr_flag;
    struct proc_struct *next;
    pushcli();
    acquire(&sched_class->lock);
    //local_intr_save(intr_flag);
    {
        current->need_resched = 0;
        if (current->state == PROC_RUNNABLE) {
            sched_class_enqueue(current);
        }
        if ((next = sched_class_pick_next()) != NULL) {
            sched_class_dequeue(next);
        }
        if (next == NULL) {
            next = idleproc;
        }
        next->runs ++;
        if (next != current) {
            proc_run(next);
        }
    }
    release(&sched_class->lock);
    popcli();
    //local_intr_restore(intr_flag);
}

//call from outside without a procession of lock
void add_timer(timer_t *timer) {
    pushcli();
    assert(!holding(&timer_table.lock));
    acquire(&timer_table.lock);
    add_timer_internal(timer);
    release(&timer_table.lock);
    popcli();
}
void del_timer(timer_t *timer) {
    pushcli();
    assert(!holding(&timer_table.lock));
    acquire(&timer_table.lock);
    del_timer_internal(timer);
    release(&timer_table.lock);
    popcli();
}

//internal call with the procession of lock
static void
add_timer_internal(timer_t *timer) {
    //bool intr_flag;
    //local_intr_save(intr_flag);
    //pushcli();
    //acquire(&timer_table.lock);
    assert(holding(&timer_table.lock));
    {
        assert(timer->expires > 0 && timer->proc != NULL);
        assert(list_empty(&(timer->timer_link)));
        list_entry_t *le = list_next(&timer_list);
        while (le != &timer_table.timer_list) {
            timer_t *next = le2timer(le, timer_link);
            if (timer->expires < next->expires) {
                next->expires -= timer->expires;
                break;
            }
            timer->expires -= next->expires;
            le = list_next(le);
        }
        list_add_before(le, &(timer->timer_link));
    }
    //release(&timer_table.lock);
    //popcli();
    //local_intr_restore(intr_flag);
}

// del timer from timer_list
static void
del_timer_internal(timer_t *timer) {
    //bool intr_flag;
    //local_intr_save(intr_flag);

    //pushcli();
    //acquire(&timer_table.lock);
    assert(holding(&timer_table.lock));
    {
        if (!list_empty(&(timer->timer_link))) {
            if (timer->expires != 0) {
                list_entry_t *le = list_next(&(timer->timer_link));
                if (le != &timer_table.timer_list) {
                    timer_t *next = le2timer(le, timer_link);
                    next->expires += timer->expires;
                }
            }
            list_del_init(&(timer->timer_link));
        }
    }
    //release(&timer_table.lock);
    //popcli();
    //local_intr_restore(intr_flag);
}

// call scheduler to update tick related info, and check the timer is expired? If expired, then wakup proc
void
run_timer_list(void) {
    bool intr_flag;
    //local_intr_save(intr_flag);
    pushcli();
    acquire(&timer_table.lock);
    {
        list_entry_t *le = list_next(&timer_table.timer_list);
        if (le != &timer_table.timer_list) {
            timer_t *timer = le2timer(le, timer_link);
            assert(timer->expires != 0);
            timer->expires --;
            while (timer->expires == 0) {
                le = list_next(le);
                struct proc_struct *proc = timer->proc;
                if (proc->wait_state != 0) {
                    assert(proc->wait_state & WT_INTERRUPTED);
                }
                else {
                    warn("process %d's wait_state == 0.\n", proc->pid);
                }
                wakeup_proc(proc);
                del_timer_internal(timer);
                if (le == &timer_table.timer_list) {
                    break;
                }
                timer = le2timer(le, timer_link);
            }
        }
        sched_class_proc_tick(current);
    }
    release(&timer_table.lock);
    popcli();
    //local_intr_restore(intr_flag);
}
