#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "trace.h"    
#include "spinlock.h" 
#include "fs.h"

struct trace_event trace_buffer[TRACE_BUF_SIZE];
int trace_buf_index = 0;
int trace_buf_count = 0; // Total events stored, max up to TRACE_BUF_SIZE

int filter = -1;

struct spinlock trace_lock; // For concurrency control

void trace_lock_init(void)
{
  initlock(&trace_lock, "trace_lock");
}

static char *syscall_names[] = {
    [SYS_fork] "fork",
    [SYS_exit] "exit",
    [SYS_wait] "wait",
    [SYS_pipe] "pipe",
    [SYS_read] "read",
    [SYS_kill] "kill",
    [SYS_exec] "exec",
    [SYS_fstat] "fstat",
    [SYS_chdir] "chdir",
    [SYS_dup] "dup",
    [SYS_getpid] "getpid",
    [SYS_sbrk] "sbrk",
    [SYS_sleep] "sleep",
    [SYS_uptime] "uptime",
    [SYS_open] "open",
    [SYS_write] "write",
    [SYS_mknod] "mknod",
    [SYS_unlink] "unlink",
    [SYS_link] "link",
    [SYS_mkdir] "mkdir",
    [SYS_close] "close",
    [SYS_trace] "trace",
    [SYS_get_trace] "get_trace",
   
};


int fetchint(uint addr, int *ip)
{
  struct proc *curproc = myproc();

  if (addr >= curproc->sz || addr + 4 > curproc->sz)
    return -1;
  *ip = *(int *)(addr);
  return 0;
}

int fetchstr(uint addr, char **pp)
{
  char *s, *ep;
  struct proc *curproc = myproc();

  if (addr >= curproc->sz)
    return -1;
  *pp = (char *)addr;
  ep = (char *)curproc->sz;
  for (s = *pp; s < ep; s++)
  {
    if (*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip)
{
  return fetchint((myproc()->tf->esp) + 4 + 4 * n, ip);
}


int argptr(int n, char **pp, int size)
{
  int i;
  struct proc *curproc = myproc();

  if (argint(n, &i) < 0)
    return -1;
  if (size < 0 || (uint)i >= curproc->sz || (uint)i + size > curproc->sz)
    return -1;
  *pp = (char *)i;
  return 0;
}


int argstr(int n, char **pp)
{
  int addr;
  if (argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_nice(void);
extern int sys_set_priority(void);    
extern int sys_lock(void);            
extern int sys_release(void);          
extern int sys_trace(void);          
extern int sys_get_trace(void);       
extern int sys_tracefilter(void);      
extern int sys_traceonlysuccess(void);
extern int sys_traceonlyfail(void);    


static int (*syscalls[])(void) = {
    [SYS_fork] sys_fork,
    [SYS_exit] sys_exit,
    [SYS_wait] sys_wait,
    [SYS_pipe] sys_pipe,
    [SYS_read] sys_read,
    [SYS_kill] sys_kill,
    [SYS_exec] sys_exec,
    [SYS_fstat] sys_fstat,
    [SYS_chdir] sys_chdir,
    [SYS_dup] sys_dup,
    [SYS_getpid] sys_getpid,
    [SYS_sbrk] sys_sbrk,
    [SYS_sleep] sys_sleep,
    [SYS_uptime] sys_uptime,
    [SYS_open] sys_open,
    [SYS_write] sys_write,
    [SYS_mknod] sys_mknod,
    [SYS_unlink] sys_unlink,
    [SYS_link] sys_link,
    [SYS_mkdir] sys_mkdir,
    [SYS_close] sys_close,
    [SYS_nice] sys_nice,
    [SYS_set_priority] sys_set_priority,         
    [SYS_lock] sys_lock,                        
    [SYS_release] sys_release,                  
    [SYS_trace] sys_trace,                       
    [SYS_get_trace] sys_get_trace,               
    [SYS_tracefilter] sys_tracefilter,            
    [SYS_traceonlysuccess] sys_traceonlysuccess, 
    [SYS_traceonlyfail] sys_traceonlyfail,       
                                                 
};


static int strcmp(const char *p, const char *q)
{
  while (*p && *p == *q)
  {
    p++;
    q++;
  }
  return (unsigned char)*p - (unsigned char)*q;
}

void syscall(void)
{
  int num;
  int retval;
  struct proc *curproc = myproc();

  num = curproc->tf->eax;

  if (num > 0 && num < NELEM(syscalls) && syscalls[num])
  {
    retval = -1;

    // Special handling for SYS_exit
    if (num == SYS_exit || num == SYS_sbrk)
    {
      if (curproc->tracing)
      {
        
        if (curproc->filtering && strcmp(syscall_names[num], curproc->filter_syscall) != 0)
        {
          retval = syscalls[num]();
          curproc->tf->eax = retval;
          return;
        }

        retval = 0;

        if (curproc->fail_only && retval != -1)
        {
          retval = syscalls[num]();
          curproc->tf->eax = retval;
          return;
        }

        
        cprintf("TRACE: pid = %d | command_name = %s | syscall = %s \n",
                curproc->pid, curproc->name, syscall_names[num]);

        acquire(&trace_lock);
        trace_buffer[trace_buf_index].pid = curproc->pid;
        safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
        safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num],
                   sizeof(trace_buffer[trace_buf_index].syscall_name));
        trace_buffer[trace_buf_index].retval = retval;
        trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
        if (trace_buf_count < TRACE_BUF_SIZE)
          trace_buf_count++;
        release(&trace_lock);
      }

      retval = syscalls[num]();
      curproc->tf->eax = retval;
      return;
    }

    retval = syscalls[num]();

    if (curproc->tracing)
    {
 
      if (curproc->fail_only && retval != -1)
      {
        curproc->tf->eax = retval;
        return;
      }

      if (curproc->success_only && retval == -1)
      {
        curproc->tf->eax = retval;
        return;
      }

      if (curproc->filtering && strcmp(syscall_names[num], curproc->filter_syscall) != 0)
      {
        curproc->tf->eax = retval;
        return;
      }

      if (num != SYS_exit || num != SYS_sbrk ) 
      {
        cprintf("TRACE: pid = %d | command_name = %s | syscall = %s | return value = %d\n",
                curproc->pid, curproc->name, syscall_names[num], retval);

        acquire(&trace_lock);
        trace_buffer[trace_buf_index].pid = curproc->pid;
        safestrcpy(trace_buffer[trace_buf_index].name, curproc->name, sizeof(curproc->name));
        safestrcpy(trace_buffer[trace_buf_index].syscall_name, syscall_names[num],
                   sizeof(trace_buffer[trace_buf_index].syscall_name));
        trace_buffer[trace_buf_index].retval = retval;
        trace_buf_index = (trace_buf_index + 1) % TRACE_BUF_SIZE;
        if (trace_buf_count < TRACE_BUF_SIZE)
          trace_buf_count++;
        release(&trace_lock);
      }
    }

    curproc->tf->eax = retval;
  }
  else
  {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}
