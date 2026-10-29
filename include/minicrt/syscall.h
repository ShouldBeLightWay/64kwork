#ifndef INTRO_MINICRT_SYSCALL_H
#define INTRO_MINICRT_SYSCALL_H

long sys_write(int fd, const void *buf, unsigned long len);
void sys_sleep_ms(long ms);

#endif
