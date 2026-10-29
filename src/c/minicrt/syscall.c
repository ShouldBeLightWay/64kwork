#include "minicrt/syscall.h"

struct timespec {
    long tv_sec;
    long tv_nsec;
};

long sys_write(int fd, const void *buf, unsigned long len)
{
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(1L), "D"((long)fd), "S"(buf), "d"(len)
                     : "rcx", "r11", "memory");
    return ret;
}

void sys_sleep_ms(long ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(35L), "D"(&ts), "S"((void *)0)
                     : "rcx", "r11", "memory");
    (void)ret;
}
