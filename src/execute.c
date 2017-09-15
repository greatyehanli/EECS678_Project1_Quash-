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
#include "quash.h"


IMPLEMENT_DEQUE_STRUCT(PIDDeque, pid_t);
IMPLEMENT_DEQUE(PIDDeque, pid_t);

// declare a global pipe
int p1[2];

typedef struct Job {
  int job_id;
  char* cmd; 
  PIDDeque pid_list;
} Job;

IMPLEMENT_DEQUE_STRUCT(JobDeque, Job);
IMPLEMENT_DEQUE(JobDeque, Job);

static JobDeque jobs;

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
  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple

  // QUESTION: What does this do?
  // Change this to true if necessary
  *should_free = true;

  char *cwd = getcwd(NULL, 0);

  // WARNING: free() needs to be used on cwd when done? 
  return cwd;

}

// Returns the value of an environment riable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple

  // TODO: Remove warning silencers
  (void) env_var; // Silence unused variable warning

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  IMPLEMENT_ME();

  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd);
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

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  // TODO: Change directory
  if(chdir(dir) == -1){
    perror("ERROR: Failed to change the dir");
  }

  // TODO: Update the PWD environment variable to be the new current working
  // directory and optionally update OLD_PWD environment variable to be the old
  // working directory.
  // Question: could I use my run_export function to do this?  
  bool should_free = true;
  char *PWD = get_current_directory(&should_free);
  setenv(PWD, cmd.dir, 1);

  return;
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  IMPLEMENT_ME();
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory
  char cwd[2000];
  getcwd(cwd, sizeof(cwd));
  fprintf(stdout, "%s\n", cwd);

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  IMPLEMENT_ME();

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

  // TODO: Setup pipes, redirects, and new process
  
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

  }
  // parent process
  else {
    // close pipes 
    close(p1[0]);
    close(p1[1]);

    parent_run_command(holder.cmd); 
  }
  
  
  // gonna do the forks and the if pid == zero
  // create 1 pipe and 1 child, if need more child creates it 

  // if/else checking if child/parent.

  // child you push job onto pid queue
  // something with jobs and looping through the queue

  

}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  CommandType type;

  Job job;
  job.cmd = get_command_string();
  // set initial size of PIE Deque at 10
  job.pid_list = new_PIDDeque(10); 

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i], &job);

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete
    IMPLEMENT_ME();
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    IMPLEMENT_ME();

    // TODO: Once jobs are implemented, uncomment and fill the following line
    // print_job_bg_start(job_id, pid, cmd);
  }
}
