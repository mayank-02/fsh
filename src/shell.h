#include <sys/types.h>
#include <unistd.h>
#include "parse.h"

/* Maximum character limit on command */
#define CMD_SIZE 1024

/* Max number of processes allowed in a group */
#define MAX_PROCS_IN_GROUP 16

/* Flags needed while reading/writing a file */
#define READ_FLAGS (O_RDONLY)
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)
#define APPEND_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define READ_MODES (S_IRUSR | S_IRGRP | S_IROTH)
#define CREATE_MODES (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* For showing colored prompt */
#define CYN   "\x1B[36m"
#define BLU   "\x1B[34m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

/* States a process or a job can be in */
typedef enum {
	FG, BG, STOPPED
} ProcState;

/* Structure to store information for a process group or a job */
typedef struct {
	/* To store all information regarding processes in that group */
	cmdTable cmdTab;
	/* Process Group ID of the job */
	pid_t pgid;
	/* Process IDs of the processes in the job */
	pid_t pids[MAX_PROCS_IN_GROUP];
	/* Number of processes running or stopped i.e. not completed */
	int numProcs;
	/* Status of job */
	ProcState status;
} job;

/* A stack of jobs with index to implement job control */
job jobsTable[16];
int jobsTableIdx = 0;

/**
 * Function to print prompt in a pretty way
 */
void printPrompt();

/**
 * Handler for SIGCHLD signal
 * @param signum signal number for corresponding signal
 */
void sigchldHandler(int signum);

/**
 * Handler for SIGINT signal or Ctrl-C
 * @param signum signal number for corresponding signal
 */
void sigintHandler(int signum);

/**
 * Handler for SIGTSTP signal or Ctrl-Z
 * @param signum signal number for corresponding signal
 */
void sigtstpHandler(int signum);

/**
 * Function to print jobs table using global array of jobs
 */
void printJobsTable();

/**
 * Initialise an empty job
 * @param cmdTab pointer to command table
 * @return initialised job 
 */
job makeJob(cmdTable *cmdTab);

/**
 * Executes commands
 * @param cmdTab pointer to command table
 */
void executor(cmdTable *cmdTab);

/**
 * Run job at top of stack, in foreground 
 */
void fg();

/**
 * Run most recent STOPPED job, in background
 */
void bg();

/**
 * Frees jobs table 
 */
void freeJobsTable();
