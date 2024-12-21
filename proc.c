#include "types.h"
// #include "user.h" 
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

extern uint ticks; // extra credit-1

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


// Define struct priority_lock
struct priority_lock
{
  int id;                  // Lock identifier
  int locked;              // Lock status (0: unlocked, 1: locked)
  int owner_pid;           // PID of the process holding the lock
  int owner_original_nice; // Original priority of the owner
};

// project 2.4
struct trace_event
{
  int pid;
  char name[16];
  char syscall_name[16];
  int retval;
};



// extra credit-1
struct lock_t
{
  int locked;         // Is the lock held?
  int id;             // Lock identifier (1-7)
  struct proc *owner; // Process holding the lock
  struct spinlock lk; // Spinlock protecting this lock
};
// extra credit-1

// Declare the locks array
// struct priority_lock locks[MAX_LOCKS]; ///if extra credit-1 not works
struct lock_t locks[MAX_LOCKS]; // extra credit-1


static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int cpuid()
{
  return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i)
  {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// PAGEBREAK: 32
//  Look in the process table for an UNUSED proc.
//  If found, change state to EMBRYO and initialize
//  state required to run in the kernel.
//  Otherwise return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->nice = 3;
  // // extra credit-1
  p->priority = 5; // Default priority
  p->original_priority = 0;
 

  // project
  p->tracing = 0;                               // Initialize tracing flag
  p->trace_buf_index = 0;                       // project 2.6
  memset(p->trace_buf, 0, PROC_TRACE_BUF_SIZE); // project 2.6
  p->filtering = 0;                             // project 3.1
  p->filter_syscall[0] = '\0';                  // project 3.1
  p->success_only = 0;                          // project 3.2
  p->fail_only = 0;                             // project 3.3
  

  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0)
  {
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// PAGEBREAK: 32
//  Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  init_locks(); // extra credit-1
  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");


  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if (n > 0)
  {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  else if (n < 0)
  {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;
  np->tracing = curproc->tracing;                // project alternative
  np->trace_buf_index = 0;                       // project 2.6
  memset(np->trace_buf, 0, PROC_TRACE_BUF_SIZE); // project 2.6


  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++)
  {
    if (curproc->ofile[fd])
    {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }


  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curproc)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}


void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu(); // Get the current CPU
  c->proc = 0;

#if PRIORITY_SCHEDULER
  struct proc *highest_p;
  int highest_priority;
#endif

  for (;;)
  {
    sti(); // Enable interrupts on this processor

    acquire(&ptable.lock);

#if PRIORITY_SCHEDULER
    highest_p = 0;
    highest_priority = 6; // Set to one higher than the lowest priority

    // Loop over the process table to find the highest priority RUNNABLE process
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;
      if (p->nice < highest_priority)
      {
        highest_priority = p->nice;
        highest_p = p;
      }
    }

    if (highest_p)
    {
      // Run the highest priority process
      p = highest_p;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&c->scheduler, p->context);

      switchkvm();
      c->proc = 0;
    }
#else
    // Original Round Robin scheduler
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;

      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&c->scheduler, p->context);

      switchkvm();
      c->proc = 0;
    }
#endif

    release(&ptable.lock);
  }
}


void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&ptable.lock))
    panic("sched ptable.lock");
  if (mycpu()->ncli != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (readeflags() & FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  acquire(&ptable.lock); // DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first)
  {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  if (lk == 0)
    panic("sleep without lk");

  
  if (lk != &ptable.lock)
  {                        // DOC: sleeplock0
    acquire(&ptable.lock); // DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock)
  { // DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}


void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [EMBRYO] "embryo",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if (p->state == SLEEPING)
    {
      getcallerpcs((uint *)p->context->ebp + 2, pc);
      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// adding this

// proc.c
int setnice(int pid, int value)
{
  struct proc *p;
  int old_value = -1;

  acquire(&ptable.lock);
  if (pid == 0)
  {
    // Change the nice value of the current process
    p = myproc();
    old_value = p->nice;
    p->nice = 3;
  }
  else
  {
    // Find the process with the given pid
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->pid == pid)
      {
        old_value = p->nice;
        p->nice = value;
        break;
      }
    }
  }
  release(&ptable.lock);

  if (old_value == -1)
    return -1; // Process not found

  return old_value; // Return the old nice value
}


void init_locks(void)
{
  int i;
  for (i = 0; i < MAX_LOCKS; i++)
  {
    locks[i].locked = 0;
    locks[i].id = i + 1;
    locks[i].owner = 0;
    initlock(&locks[i].lk, "userlock");
  }
}

struct lock_t *
get_lock(int id)
{
  if (id < 1 || id > MAX_LOCKS)
    return 0;
  return &locks[id - 1];
}

void acquire_lock(struct lock_t *lk)
{
  acquire(&lk->lk);
  while (lk->locked)
  {
    // Priority inheritance
    if (lk->owner && lk->owner->priority > myproc()->priority)
    {
      if (lk->owner->original_priority == 0)
      {
        lk->owner->original_priority = lk->owner->priority;
        cprintf("[%d ticks] Priority inversion detected. Priority Inheritance is Implemented: PID %d priority elevated from %d to %d\n",
                ticks, lk->owner->pid, lk->owner->priority, myproc()->priority);
      }
      lk->owner->priority = myproc()->priority;
    }
    // Sleep and release lk->lk; it will be re-acquired upon waking up
    sleep(lk, &lk->lk);
    // Do not re-acquire lk->lk here; sleep() already does it
  }
  lk->locked = 1;
  lk->owner = myproc();
  cprintf("[%d ticks] PID %d acquired lock %d\n", ticks, myproc()->pid, lk->id);
  release(&lk->lk);
}

void release_lock(struct lock_t *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  if (lk->owner)
  {
    if (lk->owner->original_priority != 0)
    {
      cprintf("[%d ticks] PID %d priority restored to %d\n Priority Inheritance Completed\n",
              ticks, lk->owner->pid, lk->owner->original_priority);
      lk->owner->priority = lk->owner->original_priority;
      lk->owner->original_priority = 0;
    }
    lk->owner = 0;
  }
  wakeup(lk);
  release(&lk->lk);
}
// // extra credit-1