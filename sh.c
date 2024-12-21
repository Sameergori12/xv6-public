// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"    //project 3.1
#include "fs.h"      //project 3.1
#include "syscall.h" //project 3.1
// #include "proc.h"    //project 3.1
// #include "trace.h"
// #include "strace.h" //project

// Parsed command representation
#define EXEC 1
#define REDIR 2
#define PIPE 3
#define LIST 4
#define BACK 5

#define MAXARGS 10
#define MAX_SYSCALL_NAME_LEN 16

int strace_enabled = 0; // project alternative
// int tracing = 0;            // project 3.1
// int trace_syscall_num = -1; // project 3.1
// static int filtering_active = 0; // project 3.1

struct cmd
{
  int type;
};

struct execcmd
{
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd
{
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd
{
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd
{
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd
{
  int type;
  struct cmd *cmd;
};

// project 3.1
struct syscall_entry
{
  char *name;
  int num;
};

// int syscall_name_to_number(const char *name)
// {
//   if (strcmp(name, "fork") == 0)
//     return SYS_fork;
//   if (strcmp(name, "exit") == 0)
//     return SYS_exit;
//   if (strcmp(name, "wait") == 0)
//     return SYS_wait;
//   if (strcmp(name, "pipe") == 0)
//     return SYS_pipe;
//   if (strcmp(name, "read") == 0)
//     return SYS_read;
//   if (strcmp(name, "kill") == 0)
//     return SYS_kill;
//   if (strcmp(name, "exec") == 0)
//     return SYS_exec;
//   if (strcmp(name, "fstat") == 0)
//     return SYS_fstat;
//   if (strcmp(name, "chdir") == 0)
//     return SYS_chdir;
//   if (strcmp(name, "dup") == 0)
//     return SYS_dup;
//   if (strcmp(name, "getpid") == 0)
//     return SYS_getpid;
//   if (strcmp(name, "sbrk") == 0)
//     return SYS_sbrk;
//   if (strcmp(name, "sleep") == 0)
//     return SYS_sleep;
//   if (strcmp(name, "uptime") == 0)
//     return SYS_uptime;
//   if (strcmp(name, "open") == 0)
//     return SYS_open;
//   if (strcmp(name, "write") == 0)
//     return SYS_write;
//   if (strcmp(name, "mknod") == 0)
//     return SYS_mknod;
//   if (strcmp(name, "unlink") == 0)
//     return SYS_unlink;
//   if (strcmp(name, "link") == 0)
//     return SYS_link;
//   if (strcmp(name, "mkdir") == 0)
//     return SYS_mkdir;
//   if (strcmp(name, "close") == 0)
//     return SYS_close;
//   if (strcmp(name, "trace") == 0)
//     return SYS_trace;
//   // Add other system calls as needed
//   return -1; // Unknown system call
// }

// struct syscall_entry syscall_table[] = {
//     {"fork", SYS_fork},
//     {"exit", SYS_exit},
//     {"wait", SYS_wait},
//     {"pipe", SYS_pipe},
//     {"read", SYS_read},
//     {"kill", SYS_kill},
//     {"exec", SYS_exec},
//     {"fstat", SYS_fstat},
//     {"chdir", SYS_chdir},
//     {"dup", SYS_dup},
//     {"getpid", SYS_getpid},
//     {"sbrk", SYS_sbrk},
//     {"sleep", SYS_sleep},
//     {"uptime", SYS_uptime},
//     {"open", SYS_open},
//     {"write", SYS_write},
//     {"mknod", SYS_mknod},
//     {"unlink", SYS_unlink},
//     {"link", SYS_link},
//     {"mkdir", SYS_mkdir},
//     {"close", SYS_close},
//     {"trace", SYS_trace},
//     // Add other syscalls as needed
// };

char *syscall_name[] = {
    "", // syscall 0 is unused
    "fork",
    "exit",
    "wait",
    "pipe",
    "read",
    "kill",
    "exec",
    "fstat",
    "chdir",
    "dup",
    "getpid",
    "sbrk",
    "sleep",
    "uptime",
    "open",
    "write",
    "mknod",
    "unlink",
    "link",
    "mkdir",
    "close",
    "trace",
    // Add other syscalls as needed
};
// project 3.1

int fork1(void); // Fork but panics on failure.
void panic(char *);
struct cmd *parsecmd(char *);

// project
// int strncmp(const char *s1, const char *s2, int n)
// {
//   while (n > 0 && *s1 && (*s1 == *s2))
//   {
//     s1++;
//     s2++;
//     n--;
//   }
//   if (n == 0)
//     return 0;
//   return (unsigned char)*s1 - (unsigned char)*s2;
// }
// project

// project 3.1
// int get_syscall_num(const char *name)
// {
//   int i;
//   int n = sizeof(syscall_table) / sizeof(syscall_table[0]);
//   for (i = 0; i < n; i++)
//   {
//     if (strcmp(name, syscall_table[i].name) == 0)
//       return syscall_table[i].num;
//   }
//   return -1; // Not found
// }
// project 3.1
static char *
mysafestrcpy(char *s, const char *t, int n)
{
  char *os = s;
  if (n <= 0)
    return os;
  while (--n > 0 && *t != 0)
    *s++ = *t++;
  *s = 0;
  return os;
}
// project 3.1
int strncmp(const char *s1, const char *s2, int n)
{
  while (n > 0 && *s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
    n--;
  }
  if (n == 0)
    return 0;
  return (unsigned char)*s1 - (unsigned char)*s2;
}

// project 3.1

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    exit();

  switch (cmd->type)
  {
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd *)cmd;
    if (ecmd->argv[0] == 0)
      exit();
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd *)cmd;
    close(rcmd->fd);
    if (open(rcmd->file, rcmd->mode) < 0)
    {
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd *)cmd;
    if (fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd *)cmd;
    if (pipe(p) < 0)
      panic("pipe");
    if (fork1() == 0)
    {
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if (fork1() == 0)
    {
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;

  case BACK:
    bcmd = (struct backcmd *)cmd;
    if (fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit();
}

int getcmd(char *buf, int nbuf)
{
  printf(2, "$ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if (buf[0] == 0) // EOF
    return -1;
  return 0;
}

// project 4
// static char *
// mystrstr(const char *haystack, const char *needle)
// {
//   if (*needle == '\0')
//     return (char *)haystack;

//   for (; *haystack; haystack++)
//   {
//     const char *h = haystack;
//     const char *n = needle;
//     while (*h && *n && *h == *n)
//     {
//       h++;
//       n++;
//     }
//     if (*n == '\0')
//       return (char *)haystack;
//   }
//   return 0;
// }
// static int tracing = 0;   // strace on/off
// static int filtering = 0; // set by "strace -e <syscall>", next command uses filter
// static char next_filter[16];
// project 4

int main(void)
{
  static char buf[100];
  // static int filtering_active = 0; // project 3.1
  static char next_filter[16]; // project 3.1
  int fd;
  int tracing = 0; // project

  // Ensure that three file descriptors are open.
  while ((fd = open("console", O_RDWR)) >= 0)
  {
    if (fd >= 3)
    {
      close(fd);
      break;
    }
  }

  // Read and run input commands.
  while (getcmd(buf, sizeof(buf)) >= 0)
  {
    if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
    {
      // Chdir must be called by the parent, not the child.
      buf[strlen(buf) - 1] = 0; // chop \n
      if (chdir(buf + 3) < 0)
        printf(2, "cannot cd %s\n", buf + 3);
      continue;
    }
    // project
    else if (strncmp(buf, "strace run ", 11) == 0)
    {
      // Handle 'strace run <command>'
      if (fork1() == 0)
      {
        trace(1);                   // Enable tracing in the child process
        runcmd(parsecmd(buf + 10)); // Execute the command after 'strace run '
      }
      wait();
      continue;
    }
    else if (strcmp(buf, "strace on\n") == 0)
    {
      tracing = 1; // Enable tracing for child processes
      continue;
    }
    else if (strcmp(buf, "strace off\n") == 0)
    {
      tracing = 0; // Disable tracing
      continue;
    }
    // project 2.4
    // project 3.2
    else if (strncmp(buf, "strace -s ", 10) == 0)
    {
      // Extract the command after "strace -s "
      char *p = buf + 9; // 's'(0) 't'(1) 'r'(2) 'a'(3) 'c'(4) 'e'(5) ' '(6) '-'(7) 's'(8) ' '(9)
      // p now points to the space after 's', command starts at buf+9
      // Remove trailing newline if any
      int len = strlen(p);
      if (len > 0 && p[len - 1] == '\n')
        p[len - 1] = 0;

      // Run one command with success-only filtering
      int pid = fork1();
      if (pid == 0)
      {
        // Child: set success_only and enable tracing
        traceonlysuccess(1);
        trace(1);
        runcmd(parsecmd(p));
      }
      wait();
      // After this command finishes, we do nothing special
      // success_only was set in the child only, so no resets needed
      continue;
    }
    // project 3.2
    // project 3.3
    else if (strncmp(buf, "strace -f ", 10) == 0)
    {
      char *p = buf + 9; // command starts after "strace -f "
      int len = strlen(p);
      if (len > 0 && p[len - 1] == '\n')
        p[len - 1] = 0; // remove newline

      int pid = fork1();
      if (pid == 0)
      {
        // Child process: enable fail-only mode and tracing
        traceonlyfail(1);
        trace(1);
        runcmd(parsecmd(p));
      }
      wait();
      // No reset needed; only child had fail_only set
      continue;
    }
    // project 3.3
    // project 3.4
    // project 4
    // project 4
    else if (strcmp(buf, "strace dump\n") == 0)
    {
      // Handle 'strace dump' command
      struct trace_event *events = malloc(sizeof(struct trace_event) * TRACE_BUF_SIZE);
      if (events == 0)
      {
        printf(1, "Failed to allocate memory for trace events\n");
        continue;
      }
      int n = get_trace(events, TRACE_BUF_SIZE);
      int i;
      for (i = 0; i < n; i++)
      {
        if (events[i].retval == 0 && (strcmp(events[i].syscall_name, "exit") == 0))
        {
          printf(1, "TRACE: pid = %d | command_name = %s | syscall = %s\n",
                 events[i].pid, events[i].name, events[i].syscall_name);
        }
        else
        {
          printf(1, "TRACE: pid = %d | command_name = %s | syscall = %s | return value = %d\n",
                 events[i].pid, events[i].name, events[i].syscall_name, events[i].retval);
        }
      }
      free(events);
      continue;
    }
    // project 3.1
    else if (strncmp(buf, "strace -e ", 10) == 0)
    {
      // strace -e <syscall>
      char *p = buf + 10;
      int len = strlen(p);
      if (len > 0 && p[len - 1] == '\n')
        p[len - 1] = 0; // remove newline
      mysafestrcpy(next_filter, p, sizeof(next_filter));
      continue;
    }
    // project 3.1
    // project 3.1
    if (next_filter[0] != '\0')
    {
      int pid = fork1();
      if (pid == 0)
      {
        // In child: set filter and enable tracing
        tracefilter(next_filter);
        trace(1);
        runcmd(parsecmd(buf));
      }
      wait();
      next_filter[0] = '\0'; // clear filter after one command
      continue;
    }
    // project 3.1
    // project 3.4
    // project 3.4
    //  project 2.4
    //   project
    // project 4
    // project 4
    if (fork1() == 0)
    {
      if (tracing)
        trace(1);
      runcmd(parsecmd(buf));
    }
    wait();
  }
  exit();
} // if project 3.1 does not work back to normal

// project 3.1
// int main(void)
// {
//   static char buf[100];
//   int fd;
//   // int tracing = 0;        // Global tracing flag if project 3.1 not works
//   // int trace_next_cmd = 0; // Flag to trace the next command if project 3.1 not works
//   int tracing = 0; // Global tracing flag for 'strace on' and 'strace off'
//   // char syscall_filter[16] = {0};
//   int trace_option = 0;        // Flag to indicate -e option is active
//   int traced_syscall_num = -1; // Syscall number to trace with -e option
//   // int trace_next_cmd = 0; // Flag to indicate tracing for the next command
//   // char trace_syscall[16];

//   // Ensure that three file descriptors are open.
//   while ((fd = open("console", O_RDWR)) >= 0)
//   {
//     if (fd >= 3)
//     {
//       close(fd);
//       break;
//     }
//   }

//   // Disable tracing in the shell process initially
//   // opt_trace(0, -1);

//   // Read and run input commands.
//   while (getcmd(buf, sizeof(buf)) >= 0)
//   {
//     if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
//     {
//       // Chdir must be called by the parent, not the child.
//       buf[strlen(buf) - 1] = 0; // Chop \n
//       if (chdir(buf + 3) < 0)
//         printf(2, "cannot cd %s\n", buf + 3);
//       continue;
//     }
//     else if (strcmp(buf, "strace on\n") == 0)
//     {
//       tracing = 1; // Enable tracing for shell and child processes// Enable tracing in the shell process
//       continue;
//     }
//     else if (strcmp(buf, "strace off\n") == 0)
//     {
//       tracing = 0; // Disable tracing // Disable tracing in the shell process
//       continue;
//     }
//     else if (strncmp(buf, "strace -e ", 10) == 0)
//     {
//       // Handle 'strace -e <syscall>'
//       // Extract the syscall name
//       char syscall_name[MAX_SYSCALL_NAME_LEN];
//       int i = 10; // Start after "strace -e "
//       int j = 0;
//       while (buf[i] != '\n' && buf[i] != '\0' && j < MAX_SYSCALL_NAME_LEN - 1)
//       {
//         syscall_name[j++] = buf[i++];
//       }
//       syscall_name[j] = '\0';

//       // Map syscall name to syscall number
//       int syscall_num = -1;
//       if (strcmp(syscall_name, "fork") == 0)
//         syscall_num = SYS_fork;
//       else if (strcmp(syscall_name, "exit") == 0)
//         syscall_num = SYS_exit;
//       else if (strcmp(syscall_name, "wait") == 0)
//         syscall_num = SYS_wait;
//       else if (strcmp(syscall_name, "write") == 0)
//         syscall_num = SYS_write;
//       else if (strcmp(syscall_name, "read") == 0)
//         syscall_num = SYS_read;
//       else if (strcmp(syscall_name, "open") == 0)
//         syscall_num = SYS_open;
//       else if (strcmp(syscall_name, "close") == 0)
//         syscall_num = SYS_close;
//       // Add other syscalls as needed
//       else
//       {
//         printf(2, "Invalid syscall name: %s\n", syscall_name);
//         continue;
//       }

//       traced_syscall_num = syscall_num;
//       trace_option = 1; // Enable -e option for next command
//       continue;
//     }
//     else if (strncmp(buf, "strace run ", 11) == 0)
//     {
//       // Handle 'strace run <command>'
//       if (fork1() == 0)
//       {
//         trace(1);                   // Enable tracing in the child process
//         runcmd(parsecmd(buf + 10)); // Execute the command after 'strace run '
//         exit();
//       }
//       wait();
//       continue;
//     }
//     else if (strcmp(buf, "strace dump\n") == 0)
//     {
//       // Handle 'strace dump' command
//       struct trace_event *events = malloc(sizeof(struct trace_event) * TRACE_BUF_SIZE);
//       if (events == 0)
//       {
//         printf(1, "Failed to allocate memory for trace events\n");
//         continue;
//       }
//       int n = get_trace(events, TRACE_BUF_SIZE);
//       int i;
//       for (i = 0; i < n; i++)
//       {
//         if (events[i].retval == 0 && (strcmp(events[i].syscall_name, "exit") == 0 || strcmp(events[i].syscall_name, "exec") == 0))
//         {
//           printf(1, "TRACE: pid = %d | command_name = %s | syscall = %s\n",
//                  events[i].pid, events[i].name, events[i].syscall_name);
//         }
//         else
//         {
//           printf(1, "TRACE: pid = %d | command_name = %s | syscall = %s | return value = %d\n",
//                  events[i].pid, events[i].name, events[i].syscall_name, events[i].retval);
//         }
//       }
//       free(events);
//       continue;
//     }

//     // if (fork1() == 0)
//     // {
//     // Child process
//     // if (trace_next_cmd)
//     // {
//     //   trace(1, trace_syscall_num); // Enable tracing for the specific syscall
//     // }
//     // else if (tracing)
//     // {
//     //   trace(1, -1); // Enable tracing for all syscalls
//     // }
//     // else
//     // {
//     //   trace(0, -1); // Ensure tracing is off
//     // }
//     //   runcmd(parsecmd(buf));
//     //   exit();
//     // }
//     // wait();

//     int pid = fork();
//     if (pid < 0)
//     {
//       printf(2, "fork failed\n");
//       continue;
//     }
//     if (pid == 0)
//     {
//       // Child process
//       if (trace_option)
//       {
//         // Enable tracing for the specified syscall
//         trace(1);                                   // Enable tracing
//         set_traced_syscall_num(traced_syscall_num); // Set the syscall number to trace
//         // No need to reset trace_option here; it's in the parent
//       }
//       else if (tracing)
//       {
//         // Tracing is on globally
//         trace(1);                   // Enable tracing
//         set_traced_syscall_num(-1); // -1 means trace all syscalls
//       }
//       else
//       {
//         // Tracing is off
//         trace(0);
//         set_traced_syscall_num(-1); // Reset any previous syscall filtering
//       }

//       runcmd(parsecmd(buf));
//     }
//     else
//     {
//       // Parent process
//       wait();
//       // Reset trace_option and traced_syscall_num after the command execution
//       if (trace_option)
//       {
//         trace_option = 0;
//         traced_syscall_num = -1;
//       }
//     }
//   }
//   exit();
// }

// project 3.1

void panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int fork1(void)
{
  int pid;

  pid = fork();
  if (pid == -1)
    panic("fork");
  return pid;
}

// PAGEBREAK!
//  Constructors

struct cmd *
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd *)cmd;
}

struct cmd *
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd *)cmd;
}

struct cmd *
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

struct cmd *
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

struct cmd *
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd *)cmd;
}
// PAGEBREAK!
//  Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  if (q)
    *q = s;
  ret = *s;
  switch (*s)
  {
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if (*s == '>')
    {
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if (eq)
    *eq = s;

  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *nulterminate(struct cmd *);

struct cmd *
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if (s != es)
  {
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd *
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while (peek(ps, es, "&"))
  {
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if (peek(ps, es, ";"))
  {
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd *
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if (peek(ps, es, "|"))
  {
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd *
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while (peek(ps, es, "<>"))
  {
    tok = gettoken(ps, es, 0, 0);
    if (gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch (tok)
    {
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
      break;
    case '+': // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd *
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if (!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if (!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd *
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if (peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd *)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while (!peek(ps, es, "|)&;"))
  {
    if ((tok = gettoken(ps, es, &q, &eq)) == 0)
      break;
    if (tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if (argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd *
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    return 0;

  switch (cmd->type)
  {
  case EXEC:
    ecmd = (struct execcmd *)cmd;
    for (i = 0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd *)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd *)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd *)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd *)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}