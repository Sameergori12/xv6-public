#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"    
#include "fs.h"      
#include "syscall.h" 


#define EXEC 1
#define REDIR 2
#define PIPE 3
#define LIST 4
#define BACK 5

#define MAXARGS 10
#define MAX_SYSCALL_NAME_LEN 16

int strace_enabled = 0; 

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

char *syscall_name[] = {
    "", 
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
};


int fork1(void); 
void panic(char *);
struct cmd *parsecmd(char *);

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


int main(void)
{
  static char buf[100];

  static char next_filter[16]; 
  int fd;
  int tracing = 0; 

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
    
    else if (strncmp(buf, "strace -s ", 10) == 0)
    {
      char *p = buf + 9; 
      
      int len = strlen(p);
      if (len > 0 && p[len - 1] == '\n')
        p[len - 1] = 0;

      int pid = fork1();
      if (pid == 0)
      {
        traceonlysuccess(1);
        trace(1);
        runcmd(parsecmd(p));
      }
      wait();
      continue;
    }
   
    else if (strncmp(buf, "strace -f ", 10) == 0)
    {
      char *p = buf + 9; // command starts after "strace -f "
      int len = strlen(p);
      if (len > 0 && p[len - 1] == '\n')
        p[len - 1] = 0; // remove newline

      int pid = fork1();
      if (pid == 0)
      {
        traceonlyfail(1);
        trace(1);
        runcmd(parsecmd(p));
      }
      wait();
      continue;
    }
  
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
    
    if (fork1() == 0)
    {
      if (tracing)
        trace(1);
      runcmd(parsecmd(buf));
    }
    wait();
  }
  exit();
} 

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
    case '+':
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