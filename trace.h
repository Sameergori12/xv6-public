// trace.h
#ifndef KERNEL_TRACE_H
#define KERNEL_TRACE_H

#define TRACE_BUF_SIZE 4096 // Configurable size of the buffer

struct trace_event
{
    int pid;
    char name[16];
    char syscall_name[16];
    int retval;
};

#endif // KERNEL_TRACE_H
