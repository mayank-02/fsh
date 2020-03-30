#define ARGS_SIZE 1024
#define MAX_CMDS 32

/* For debugging purposes */
#define DEBUG 0
#define debug_printf(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

/* To check type of character */
#define IS_NULL(c) 			({(int)(c) == '\0';})
#define IS_INPUT(c) 		({(int)(c) == '<';})
#define IS_OUTPUT(c) 		({(int)(c) == '>';})
#define IS_PIPE(c) 			({(int)(c) == '|';})
#define IS_WHITESPACE(c) 	({(int)(c) == ' ';})
#define IS_AMPERSAND(c) 	({(int)(c) == '&';})
#define IS_NORMAL(c) 		((!IS_NULL(c)) && (!IS_INPUT(c)) && (!IS_OUTPUT(c)) && \
							(!IS_WHITESPACE(c)) && (!IS_PIPE(c)) && (!IS_AMPERSAND(c)))

/**
 * Command table to store all information regarding commands,
 * their redirection files, and if background or not
 */
typedef struct {
	char *cmdLine;
	char *args[MAX_CMDS][ARGS_SIZE];
	char *infile;
	char *outfile;
	bool isbackground;
	int numCmds;
} cmdTable;

/* States of finite state machine to parse command */
typedef enum {
	INIT, ARGS, CMD, SPECIAL, AMPERSAND, FILENAME
} State;

/* Types of arguments in any command */
typedef enum {
	COMMAND, INFILE, OUTFILE
} ArgType;


/**
 * Initialise the command table
 * @param cmdTab Pointer to command table
 */
void initCmdTable(cmdTable *cmdTab);

/**
 * Frees all memory allocated while making command table
 * @param cmdTab Pointer to command table
 */
void freeCmdTable(cmdTable *cmdTab);

/**
 * Function to initialise command table using command line string
 * @param cmdLine user input command
 * @param cmdTab Pointer to command table
 */
void parse(char *cmdLine, cmdTable *cmdTab);

/**
 * Function to print out command table
 * @param cmdTab Pointer to command table
 */
void printCmdTable(cmdTable *cmdTab);