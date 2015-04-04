#include <stdio.h>
#include <ulib.h>
#include "syscall.h"

const int max_child = 32;

int
main(void) {
    int n, pid;
        cprintf("I am master, I run on cpu %d\n",sys_getcpuid());

    for (n = 0; n < max_child; n++) {
        if ((pid = fork()) == 0) {
            cprintf("I am child %d, I run on cpu %d\n", n, sys_getcpuid());
            exit(0);
        }
        assert(pid > 0);
    }
    if (n > max_child) {
        panic("fork claimed to work %d times!\n", n);
    }

    for (; n > 0; n --) {
        if (wait() != 0) {
            panic("wait stopped early\n");
        }
    }

    if (wait() == 0) {
        panic("wait got too many\n");
    }

    cprintf("forktest pass.\n");
    return 0;
}
