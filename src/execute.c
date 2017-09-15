/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "quash.h"


IMPLEMENT_DEQUE_STRUCT(PIDDeque, pid_t);
IMPLEMENT_DEQUE(PIDDeque, pid_t);

// declare a global pipe
int p1[2];

typedef struct Job {
  int job_id;
  char* cmd; 
  PIDDeque pid_list;
  bool isComplete;
} Job;

IMPLEMENT_DEQUE_STRUCT(JobDeque, Job);
IMPLEMENT_DEQUE(JobDeque, Job);

static JobDeque jobs;
bool isJobDequeInit = false;

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // HINT: This should be pretty simple

  // Change this to true if necessary

  char *cwd = getcwd(NULL, 0);

  // QUESTION: What does this do?
  *should_free = true;  

  // WARNING: free() needs to be used on cwd when done? 
  return cwd;

}

// Returns the value of an environment riable env_var
const char* lookup_env(const char* env_var) {
  // Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple

  // Remove warning silencers
  //(void) env_var; // Silence unused variable warning

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.

  for(int j = 0; j < (int)length_JobDeque(&jobs); j++) {
    
    // pop a job
    Job tempJob = pop_front_JobDeque(&jobs);
    
    // Assume job is complete    
    tempJob.isComplete = true;

    // Check the processes of the job to see if they're complete
    for(int p = 0; p < (int)length_PIDDeque(&tempJob.pid_list); p++) {
    
      int status = 0;    

      // check if process is complete
      // NOTE: check if there's a simpler way to do this
      
      // pop first process off of the temp job's process queue
      pid_t tempProcess = pop_front_PIDDeque(&tempJob.pid_list);

      if(!(waitpid(tempProcess,&status,WNOHANG) != 0 && (WIFEXITED(status) 
        || WIFSIGNALED(status)))) {

        tempJob.isComplete = false;
      }    
    
      // push process back to end of process queue
      push_back_PIDDeque(&tempJob.pid_list, tempProcess);
    }

    if(!(tempJob.isComplete)) {
      push_back_JobDeque(&jobs, tempJob);
    }
    else {
      // print that job is complete
      print_job_bg_complete(tempJob.job_id, peek_front_PIDDeque(&tempJob.pid_list), tempJob.cmd); 
      
      // destroy PID list associated with specific job      
      destroy_PIDDeque(&tempJob.pid_list);
    }
  }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  // TODO: Remove warning silencers
  //(void) exec; // Silence unused variable warning
  //(void) args; // Silence unused variable warning

  // TODO: Implement run generic
  execvp(exec, args);

  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // TODO: Remove warning silencers
  // (void) str; // Silence unused variable warning

  // TODO: Implement echo
  if(*str != NULL) {
    printf("%s", *str);
    str++;
    
    while(*str != NULL) {
      printf(" %s", *str);
      str++;
    }
  }
  printf("\n");

  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // TODO: Remove warning silencers
  //(void) env_var; // Silence unused variable warning
  //(void) val;     // Silence unused variable warning

  // Overwrite value set to 1, so will possibly overwrite existing definition
  setenv(env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;
  char* fulldir = realpath(dir, NULL);

  // Check if the directory is valid
  if (fulldir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  // Change directory, perhaps try without an error?
  chdir(fulldir);

  //bool should_free = true;
  //char *PWD = get_current_directory(&should_free);
  setenv("PWD", fulldir, 1);

  // free memory
  free(fulldir);
  //free(PWD);

  return;
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // Remove warning silencers
  //(void) signal; // Silence unused variable warning
  //(void) job_id; // Silence unused variable warning

  // Note: currently killing first process of first job, do I need to use job associated with job_id?
  // Kill all processes associated with a background job
  Job jobToKill = peek_front_JobDeque(&jobs);
  pid_t processToKill = peek_front_PIDDeque(&jobToKill.pid_list);
  kill(processToKill, signal);
}


// Prints the current working directory to stdout
void run_pwd() {
  char cwd[2000];
  getcwd(cwd, sizeof(cwd));
  fprintf(stdout, "%s\n", cwd);

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {

  // Note: does job need to be referenced here? Could i use size_t instead of int?
  for(int x = 0; x < (int)length_JobDeque(&jobs); x++) {
         
    Job tempJob = pop_front_JobDeque(&jobs);

    // TODO: put temp job above this line, use it instead of a million peeks 
    print_job(tempJob.job_id, peek_front_PIDDeque(&tempJob.pid_list), tempJob.cmd);

    // Keep correct order of queue while printing 
    push_back_JobDeque(&jobs, tempJob);
 
  } 

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, Job* job) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  //(void) p_in;  // Silence unused variable warning
  //(void) p_out; // Silence unused variable warning
  //(void) r_in;  // Silence unused variable warning
  //(void) r_out; // Silence unused variable warning
  //(void) r_app; // Silence unused variable warning
  
  // fork process
  pid_t pid_1 = fork(); 

  // push process onto job's pid list, **check that passing by reference works
  // NOTE: Look into why this ampersand helps, C thing??
  push_back_PIDDeque(&job->pid_list, pid_1); 

  // check if process is a child process
  if (pid_1 == 0) {
    if(p_in) {
      // open pipe for reading
      dup2(p1[0], STDIN_FILENO);

      // close the pipe NOTE: does it make a difference to close outside of this if
      close(p1[0]);
    }
    if(p_out) {
      // open pipe for writing
      dup2(p1[1], STDOUT_FILENO);

      // close the pipe NOTE: does it make a difference to close outside of this if
      close(p1[1]);
    }
    if(r_in) {
      int fileDescriptor = open(holder.redirect_in, O_RDONLY, 0); // is mode 0 read only?
      
      // reading from a file
      dup2(fileDescriptor, STDIN_FILENO);
      close(fileDescriptor);

    }
    if(r_out) {
      int fileDescriptor;
 
      if(r_app) {
        // NOTE: might not need O_CREAT 
        fileDescriptor = open(holder.redirect_out, O_CREAT|O_WRONLY|O_APPEND, 0644); // pass mode as read&write, or write only?
      }
      else {
        fileDescriptor = open(holder.redirect_out, O_CREAT|O_WRONLY|O_TRUNC, 0644); // pass mode as read&write, or write only?
      }

      // writing to a file and close the pipe
      dup2(fileDescriptor, STDOUT_FILENO);
      close(fileDescriptor);
    }

    child_run_command(holder.cmd); // This should be done in the child branch of a fork
    exit(0);
  }
  // parent process
  else {
    // close pipes 
    close(p1[0]);
    close(p1[1]);

    parent_run_command(holder.cmd); 

    // why not exit?
  }

}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;

  // initialize jobs queue
  if(!isJobDequeInit) {
    jobs = new_JobDeque(10); // set initial size of Job Deque to 10  
    isJobDequeInit = true;
  }

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  CommandType type;

  Job job;
  job.cmd = get_command_string();
  job.pid_list = new_PIDDeque(10); // set initial size of PIE Deque at 10 

  int status = 0;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i], &job);

  if (!(holders[0].flags & BACKGROUND)) {
    // Run foreground job
    if(!is_empty_PIDDeque(&job.pid_list)) {
      waitpid(peek_front_PIDDeque(&job.pid_list), &status, 0);
    }
    
    // free memory
    free(job.cmd);
    destroy_PIDDeque(&job.pid_list);
  }
  else {
    // A background job.

    // Set the job id for our new job
    if(is_empty_JobDeque(&jobs)) {
      job.job_id = 1;
    }
    else {
      // check if need to pass as a reference
      job.job_id = peek_back_JobDeque(&jobs).job_id + 1;    
    }

    // Push our job onto the job queue
    push_back_JobDeque(&jobs, job);

    print_job_bg_start(job.job_id, peek_front_PIDDeque(&job.pid_list), job.cmd);
  }
}
