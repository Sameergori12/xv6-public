// #include "trace.h" //project 2.4

struct stat;
struct rtcdate;

// project 2.4

#define TRACE_BUF_SIZE 150
struct trace_event
{
    int pid;
    char name[16];
    char syscall_name[16];
    int retval;
};
// project 2.4

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int *);
int write(int, const void *, int);
int read(int, void *, int);
int close(int);
int kill(int);
int exec(char *, char **);
int open(const char *, int);
int mknod(const char *, short, short);
int unlink(const char *);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int mkdir(const char *);
int chdir(const char *);
int dup(int);
int getpid(void);
char *sbrk(int);
int sleep(int);
int uptime(void);
int lock(int id);      // extra credit
int release(int id);   // extra credit
int set_priority(int); // extra credit-1
int trace(int enable); //  if project 3.1 not works back to normal
// int trace(int enable);                              // project 3.1
// int set_traced_syscall_num(int syscall_num);        // ptroject 3.1
// int set_selective_trace(int syscall_num);           // project 3.1
// int set_selective_trace_once(int flag);             // project 3.1
int get_trace(struct trace_event *events, int max); // project 2.4
// int trace_set_filter(char *syscall_name);           // project 3.1
int tracefilter(const char *sc);  // project 3.1
int traceonlysuccess(int enable); // project 3.2
int traceonlyfail(int enable);    // project 3.3

// ulib.c
int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int strcmp(const char *, const char *);
void printf(int, const char *, ...);
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);
void *malloc(uint);
void free(void *);
int atoi(const char *);
int nice(int pid, int value);
// int lock(int id);               // extra credit
// int release(int id);            // extra credit
// int set_priority(int priority); // extra credit-1
//  int setpriority(int pid, int priority); // extra credit-1
//  int getpriority(int pid);               // extra credit-1
//  int syscall(int num, ...);              // extra credit-1