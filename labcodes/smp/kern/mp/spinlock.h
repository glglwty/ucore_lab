// Mutual exclusion lock.
#ifndef SPINLOCK_H
#define SPINLOCK_H
struct spinlock {
  uint32_t locked;       // Is the lock held?
  
  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  uint32_t pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
};

void pushcli(void);
void popcli(void);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
void initlock(struct spinlock *lk, char *name);
int holding(struct spinlock *lock);

#define try_get_save(lock, state) do{if (((state) = holding(lock)) == 0) {acquire(lock);}} while(0);
#define lock_state_resotre(lock, state) do{if ((state) == 0){release(lock);}}while(0);

#endif