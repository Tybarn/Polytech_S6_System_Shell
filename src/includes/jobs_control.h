#ifndef JOBS_CONTROL_H_
#define JOBS_CONTROL_H_

/* A process is a single process.  */
typedef struct process
{
  struct process *next;       /* next process in pipeline */
  char **argv;                /* for exec */
  pid_t pid;                  /* process ID */
  char completed;             /* true if process has completed */
  char stopped;               /* true if process has stopped */
  int status;                 /* reported status value */
} process;


/* A job is a pipeline of processes.  */
typedef struct job
{
  struct job *next;           /* next active job */
  char *command;              /* command line, used for messages */
  process *first_process;     /* list of processes in this job */
  pid_t pgid;                 /* process group ID */
  char notified;              /* true if user told about stopped job */
  struct termios tmodes;      /* saved terminal modes */
  int stdin, stdout, stderr;  /* standard i/o channels */
} job;


void init_shell();
void launch_job( job *j, int foreground);
void do_job_notification(void);
void continue_job (job *j, int foreground);
job * find_job (pid_t pgid);
int job_is_completed (job *j);
int job_is_stopped (job *j);
int job_is_stopped(job *j);
int job_is_completed(job *j);
void launch_process( process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground);
void put_job_in_foreground(job *j, int cont);
void put_job_in_background(job *j, int cont);
int mark_process_status(pid_t pid, int status);
void update_status(void);
void wait_for_job(job *j);
void format_job_info(job *j, const char *status);
void mark_job_as_running(job *j);
void free_job(job* j);

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;
/* The active jobs are linked into a list.  This is its head.   */
job *first_job;
job *stopped_now;

#endif
