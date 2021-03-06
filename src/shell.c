#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <pwd.h>
#include "shell.h"

/**
 * @brief Prints a pretty prompt 
 * 
 * Prints a pretty prompt consisting of username, hostname and 
 * current working directory
 */
void printPrompt() {
	char cwd[PATH_MAX];
	if(getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd");
		exit(EXIT_FAILURE);
	}
	
	char hostname[HOST_NAME_MAX + 1];
	gethostname(hostname, HOST_NAME_MAX + 1);
	
	struct passwd *pw = getpwuid(geteuid());
	if(pw == NULL) {
		perror("getpwuid");
	}

	printf("╭─");
	printf(CYN "%s@%s " RESET, pw->pw_name, hostname);
	printf(BLU "[%s]\n" RESET, cwd);
	printf("╰─");
	printf(GRN "$ " RESET);
	
	return;
}

/**
 * @brief Handler for SIGCHLD
 * 
 * Handles the termination of child processes by updating status in jobs table
 * 
 * @param signum Integer
 */
void sigchldHandler(int signum) {
	int status;
	pid_t temppid = waitpid(-1, &status, WNOHANG);
	if (temppid <= 0)
		return;

	if(WIFEXITED(status)) {
		for(int i = 0; i < jobsTableIdx; i++) {
			for(int j = 0; j < MAX_PROCS_IN_GROUP; j++) {
				if(temppid == jobsTable[i].pids[j]) {
					jobsTable[i].numProcs--;
					break;
				}
			}

			if(jobsTable[i].numProcs == 0) {
				printf("Done\t\tPGID [%d]\t\t\"%s\"\n", jobsTable[i].pgid, jobsTable[i].cmdTab.cmdLine);
				for(int j = i + 1; j < jobsTableIdx; j++) {
					jobsTable[j - 1] = jobsTable[j];
				}
				jobsTableIdx--;
			}
		}
	}
}

/**
 * @brief Handler for SIGINT
 * 
 * Ignores the signal and prints prompt again
 * 
 * @param signum Integer
 */
void sigintHandler(int signum) {
	printf("\n");
	printPrompt();
    fflush(stdout);
}

/**
 * @brief Handler for SIGTSTP
 * 
 * Ignores the signal and prints prompt again
 * 
 * @param signum Integer
 */
void sigtstpHandler(int signum) {
	printf("\n");
	printPrompt();
    fflush(stdout);
}

/**
 * @brief Prints jobs table
 * 
 */
void printJobsTable() {
	if(jobsTableIdx == 0) {
		printf("No background or stopped jobs\n");
		return;
	}

	printf(" PGID \t  Status \tCommand\n");
	for(int i = 0; i < jobsTableIdx; i++) {
		switch(jobsTable[i].status) {
		case FG:
			printf("[%d]\t  Foreground\t%s\n", jobsTable[i].pgid, jobsTable[i].cmdTab.cmdLine);
			break;
		case BG:
			printf("[%d]\t  Running\t%s\n", jobsTable[i].pgid, jobsTable[i].cmdTab.cmdLine);
			break;
		case STOPPED:
			printf("[%d]\t  Stopped\t%s\n", jobsTable[i].pgid, jobsTable[i].cmdTab.cmdLine);
			break;
		}
	}
}

/**
 * @brief Makes a job out of a command table
 * 
 * @param cmdTab Pointer to command table
 * @return job
 */
job makeJob(cmdTable *cmdTab) {
	job temp;
	temp.cmdTab = *cmdTab;
	temp.pgid = 0;
	for(int i = 0; i < MAX_PROCS_IN_GROUP; i++)
		temp.pids[i] = 0;
	temp.numProcs = 0;
	return temp;
}

/**
 * @brief Executes the job
 * 
 * Given a command table, makes a job out of it and then executes it using
 * fork exec semantics after doing needed redirection / piping.
 * It waits job was a foreground process.
 * 
 * @param cmdTab Pointer to command table
 */
void executor(cmdTable *cmdTab) {
	pid_t pid, pgid, tmp;
	int infd, outfd, status;
	int numPipes = cmdTab->numCmds - 1;
	int pfd[2 * cmdTab->numCmds + 1];
	job temp = makeJob(cmdTab);

	/* Make all the pipes needed for the commands */
	for(int i = 0; i < cmdTab->numCmds; i++) {
		if(pipe(pfd + i*2) < 0) {
			perror("pipe :");
			exit(EXIT_FAILURE);
		}
	}

	for(int i = 0; i < cmdTab->numCmds; i++) {
		/* Set signal handlers for parent */
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGCHLD, sigchldHandler);

		if((pid = fork()) == -1) {
			/* Fork failed */
			perror("fork: ");
			exit(EXIT_FAILURE);
		}
		else if(pid == 0) {
			/* Child process */

			/* Restore default signal handlers */
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);
			signal(SIGTTIN, SIG_DFL);
			signal(SIGTTOU, SIG_DFL);

			/* Setting same group pid for entire process group */
			if(i == 0) {
				pgid = getpid();
			}
			setpgid(0, pgid);
			temp.pids[i] = pid;
			temp.numProcs++;

			/* Set input file if first process */
			if(i == 0 && cmdTab->infile) {
				infd = open(cmdTab->infile, READ_FLAGS, READ_MODES);
				if(infd == -1) {
					perror("input file :");
					exit(EXIT_FAILURE);
				}
				dup2(infd, 0);
				close(infd);
			}

			/* Set output file if last process */
			if(i == numPipes && cmdTab->outfile) {
				outfd = open(cmdTab->outfile, CREATE_FLAGS, CREATE_MODES);
				if(outfd == -1) {
					perror("output file :");
					exit(EXIT_FAILURE);
				}
				dup2(outfd, 1);
				close(outfd);
			}

			/* Child gets input from previous process if it's not first process */
			if(i != 0) {
				if(dup2(pfd[2*i - 2], 0) < 0) {
					perror("pipe input: ");
					exit(EXIT_FAILURE);
				}
			}

			/* Child outputs to next process if it's not last process */
			if(i != numPipes) {
				if(dup2(pfd[(2*i + 1)], 1) < 0) {
					perror("pipe output: ");
					exit(EXIT_FAILURE);
				}
			}

			/* Close all pfds */
			for(int j = 0; j < (2 * numPipes); j++) {
				close(pfd[j]);
			}

			/* Execute the process finally */
			if((execvp(cmdTab->args[i][0], cmdTab->args[i])) == -1) {
				perror(cmdTab->args[i][0]);
				freeCmdTable(cmdTab);
				exit(EXIT_FAILURE);
			}
		}
		else {
			/* Parent process */
			close(pfd[2*i + 1]);
			close(pfd[2*i - 2]);

			/* Set pgid's of all processes to pid of first process in job */
			if(i == 0) {
				pgid = pid;
			}
			setpgid(pid, pgid);

			/* Add child pids to list of pids in job and incrmement count */
			temp.pids[i] = pid;
			temp.numProcs++;

			/* Add job to job table after launching last process of that job */
			if(i == numPipes) {
				if(cmdTab->isbackground)
					temp.status = BG;
				else
					temp.status = FG;

				temp.pgid = pgid;
				jobsTable[jobsTableIdx++] = temp;
			}
		}
	}

	/* Close all pfds */
	for(int j = 0; j < (2 * numPipes); j++) {
		close(pfd[j]);
	}

	/* Wait only if process group is running in foreground */
	if(!cmdTab->isbackground) {
		/* Set process group to foreground */
		tcsetpgrp(STDIN_FILENO, pgid);

		/* Wait for all processes in process group */
		for(int j = 0; j < numPipes + 1; j++) {
			if((tmp = waitpid(-pgid, &status, WUNTRACED)) == -1) {
				if(errno == ECHILD)
					break;
				perror("executor: waitpid");
			}
			if(WIFEXITED(status)) {
				/* Decrease count of running processes */
				jobsTable[jobsTableIdx - 1].numProcs--;
				if(jobsTable[jobsTableIdx - 1].numProcs == 0) {
					jobsTableIdx--;
				}
			}
			else if(WIFSIGNALED(status)) {
				/* Decrease count of running processes */
				jobsTableIdx--;
				break;
			}
			else if(WIFSTOPPED(status)) {
				/* Change status because stop signal was received */
				jobsTable[jobsTableIdx - 1].status = STOPPED;
				break;
			}
		}
		/* Remove process group from job table if no more processes are running */
		if(jobsTable[jobsTableIdx - 1].numProcs == 0) {
			jobsTableIdx--;
		}

		/* Set shell to foreground again */
		tcsetpgrp(STDIN_FILENO, getpgid(getpid()));
	}

	return;
}

/**
 * @brief Bring the most recent stopped / background job to foreground
 * 
 */
void fg() {

	/* Return if no background or stopped jobs */
	if(jobsTableIdx <= 0) {
		printf("No background or stopped jobs to send to foreground\n");
		return;
	}

	pid_t tmp, pgid = jobsTable[jobsTableIdx - 1].pgid;
	int status;

	/* Set handlers */
	signal(SIGINT, sigintHandler);
	signal(SIGTSTP, sigtstpHandler);

	/* Send stop signal to job before setting it to foreground */
	kill(-pgid, SIGTSTP);

	/* Set process group to foreground */
	tcsetpgrp(STDIN_FILENO, pgid);

	/* Send continue signal to process group */
	kill(-pgid, SIGCONT);

	/* Wait for process group */
	for(int i = 0; i < jobsTable[jobsTableIdx - 1].numProcs; i++) {
		if((tmp = waitpid(-pgid, &status, WUNTRACED)) == -1) {
			if(errno == ECHILD)
				continue;
			perror("fg: waitpid");
		}
		if(WIFEXITED(status)) {
			/* Decrease count of running processes */
			jobsTable[i].numProcs--;
			/* Remove process group from job table if no more processes are running */
			if(jobsTable[i].numProcs == 0) {
				jobsTableIdx--;
			}
		}
		else if(WIFSIGNALED(status)) {
			/* Decrease count of running processes */
			jobsTableIdx--;
			break;
		}
		else if(WIFSTOPPED(status)) {
			/* Change status because stop signal was received */
			jobsTable[jobsTableIdx - 1].status = STOPPED;
		}
	}
	/* Remove process group from job table if no more processes are running */
	if(jobsTable[jobsTableIdx - 1].numProcs == 0) {
		jobsTableIdx--;
	}

	signal(SIGTTOU, SIG_IGN);

	/* Set shell to foreground */
	tcsetpgrp(STDIN_FILENO, getpgid(getpid()));
}

/**
 * @brief Runs the most recent stopped job in background
 * 
 */
void bg() {

	/* Check if there are background or stopped jobs if any */
	if(jobsTableIdx <= 0) {
		printf("No stopped jobs to send to background\n");
		return;
	}

	for(int i = jobsTableIdx - 1; i >= 0; i--) {
		/* Find most recent STOPPED job */
		if(jobsTable[i].status == STOPPED) {
			/* Send continue signal */
			if(kill(-jobsTable[i].pgid, SIGCONT) < 0) {
				perror("bg: ");
			}

			/* Update status in job table */
			jobsTable[i].status = BG;
			break;
		}
	}
}

/**
 * @brief Frees the global job table
 * 
 */
void freeJobsTable() {
	for(int i = 0; i < jobsTableIdx; i++) {
		freeCmdTable(&jobsTable[i].cmdTab);
	}
	return;
}

/**
 * @brief Main function
 * 
 * Accepts a command line as input, parses it, executes it and then
 * finally frees it for every command until shell has exited.
 * 
 * @return int 0 if success, errno on failure
 */
int main() {
	char *cmdLine = malloc(CMD_SIZE * sizeof(char));

	/* To handle Ctrl+C and Ctrl+Z signals */
	signal(SIGINT,  sigintHandler);
   	signal(SIGTSTP, sigtstpHandler);
   	signal(SIGCHLD, sigchldHandler);

	while (1) {
		printPrompt();

		fgets(cmdLine, 1024, stdin);
		cmdLine[strcspn(cmdLine, "\n")] = '\0';
		if(!strlen(cmdLine))
			continue;

		/* Check for builtin commands */
		if(strcmp(cmdLine, "exit") == 0) {
			break;
		}
		else if(strcmp(cmdLine, "fg") == 0) {
			fg();
			continue;
		}
		else if(strcmp(cmdLine, "bg") == 0) {
			bg();
			continue;
		}
		else if(strcmp(cmdLine, "jobs") == 0) {
			printJobsTable();
			continue;
		}
		else if(strncmp(cmdLine, "cd ", 3) == 0) {
			char *token = strtok(cmdLine, " ");
			token = strtok(NULL, " ");
			if(chdir(token) == -1) {
				perror("cd");
			}
			continue;
		}

		cmdTable *cmdTab = calloc(1, sizeof(cmdTable));
		initCmdTable(cmdTab);
		parse(cmdLine, cmdTab);
		executor(cmdTab);
		free(cmdTab);
	}

	freeJobsTable();
	return 0;
}