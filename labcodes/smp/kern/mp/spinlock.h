// Mutual exclusion lock.
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