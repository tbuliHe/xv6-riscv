#include "kernel/types.h"
#include "user/user.h"

struct spinlock {
    uint locked;
};

void initlock(struct spinlock *lk) {
    lk->locked = 0;
}

void acquire(struct spinlock *lk) {
    while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ; 
}

void release(struct spinlock *lk) {
    __sync_lock_release(&lk->locked);
}

struct spinlock proclock;

int main(void) {
    initlock(&proclock);

    acquire(&proclock);
    int num = getprocnum();
        printf("Number of processes: %d\n", num);
    int pid = fork();
    if (pid < 0) {
        printf("Fork failed\n");
        release(&proclock);
        exit(1);
    } else if (pid == 0) {
        // kid proc
        num = getprocnum();
        printf("Number of processes: %d\n", num);
        release(&proclock);
        exit(0);
    } else {
        // parent proc
        wait(0);
        release(&proclock);
    }
    exit(0);
}
