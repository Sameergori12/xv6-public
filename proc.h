
#define MAX_LOCKS 7           
#define LOW_PRIORITY 1          
#define HIGH_PRIORITY 10         
#define PROC_TRACE_BUF_SIZE 4096 
#define TRACE_BUF_SIZE 4096      

struct cpu
{
  uchar apicid;              
  struct context *scheduler; 
  struct taskstate ts;       
  struct segdesc gdt[NSEGS];
  volatile uint started;     
  int ncli;                  
  int intena;                
  struct proc *proc;         
};

int setnice(int pid, int value);
extern struct cpu cpus[NCPU];
extern int ncpu;
void init_locks(void); 

struct context
{
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate
{
  UNUSED,
  EMBRYO,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
};

struct proc
{
  uint sz;                    
  pde_t *pgdir;              
  char *kstack;              
  enum procstate state;       
  int pid;                   
  struct proc *parent;     
  struct trapframe *tf;
  struct context *context; 
  void *chan;                 
  int killed;                
  struct file *ofile[NOFILE];
  struct inode *cwd;          
  char name[16];              
  int nice;                  
  int original_nice;          
  int has_inherited;         

  int priority;
  int original_priority; 
  int tracing; 
  char trace_buf[TRACE_BUF_SIZE];           
  char proc_trace_buf[PROC_TRACE_BUF_SIZE]; 
  int trace_buf_index;                     
  int filtering;                           
  char filter_syscall[16];                 
  int success_only;                        
  int fail_only;                          
};


struct lock_t;

void init_locks(void);
struct lock_t *get_lock(int id);
void acquire_lock(struct lock_t *lk);
void release_lock(struct lock_t *lk);

