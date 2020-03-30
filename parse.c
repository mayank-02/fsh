#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parse.h"

void initCmdTable(cmdTable *cmdTab) {
	debug_printf("%s\n", "initCmdTable: Entered");

	for(int i = 0; i < MAX_CMDS; i++)
		for(int j = 0; j < ARGS_SIZE; j++)
			cmdTab->args[i][j] = NULL;

	cmdTab->cmdLine = NULL;
	cmdTab->infile = NULL;
	cmdTab->outfile = NULL;
	cmdTab->isbackground = false;
	cmdTab->numCmds = 0;

	debug_printf("%s\n", "initCmdTable: Exited");
}

void freeCmdTable(cmdTable *cmdTab) {
	debug_printf("%s\n", "freeCmdTable: Entered");

	int argsRow, argsCol;
	for(argsRow = 0; argsRow < cmdTab->numCmds; argsRow++) {
		argsCol = 0;
		while(cmdTab->args[argsRow][argsCol]) {
			free(cmdTab->args[argsRow][argsCol]);
			argsCol++;
		}
	}
	free(cmdTab->cmdLine);
	free(cmdTab->infile);
	free(cmdTab->outfile);

	debug_printf("%s\n", "freeCmdTable: Exited");
}

void parse(char *cmdLine, cmdTable *cmdTab) {
	debug_printf("parse: %s\n", cmdLine);

	register char c;
	int i = 0, tokenIdx = 0, argsRow = 0, argsCol = 0;
	char *token = malloc(1024 * sizeof(char));
	State currentState = INIT;
	ArgType argExpected = COMMAND;
	cmdTab->cmdLine = strdup(cmdLine);

	while(1) {
		c = cmdLine[i];
		debug_printf("parse: char %c currentState %d\n", c, currentState);

		switch(currentState) {
			case INIT:
				if(IS_NORMAL(c)) {
					token[tokenIdx++] = c;
					currentState = ARGS;
				}
				else if(IS_WHITESPACE(c)){
					currentState = INIT;
				}
				else {
					printf("Parse Error: Unexpected syntax encountered.\n");
					debug_printf("%s\n", "parse: Exited");
					free(token);
					freeCmdTable(cmdTab);
					return;
				}
				break;

			case ARGS:
				if(IS_WHITESPACE(c)) {
					token[tokenIdx] = '\0';
					debug_printf("parse: token <%s> formed\n", token);

					cmdTab->args[argsRow][argsCol] = strdup(token);
					argsCol++;
					tokenIdx = 0;
					currentState = CMD;
				}
				else if(IS_INPUT(c) || IS_OUTPUT(c) || IS_PIPE(c)) {
					token[tokenIdx] = '\0';
					debug_printf("parse: token <%s> formed\n", token);
					cmdTab->args[argsRow][argsCol] = strdup(token);
					argsCol++;
					tokenIdx = 0;
					currentState = SPECIAL;
					if(IS_INPUT(c))
						argExpected = INFILE;
					if(IS_OUTPUT(c))
						argExpected = OUTFILE;
					if(IS_PIPE(c)) {
						currentState = ARGS;
						argsCol = 0;
						argsRow++;
					}
				}
				else if(IS_NORMAL(c)) {
					token[tokenIdx++] = c;
				}
				else if(IS_NULL(c)) {
					token[tokenIdx] = '\0';
					debug_printf("parse: token <%s> formed\n", token);

					cmdTab->args[argsRow][argsCol] = strdup(token);
					argsCol = 0;
					tokenIdx = 0;
					argsRow++;
					cmdTab->numCmds = argsRow;
					free(token);
					debug_printf("%s %d\n", "parse: Exited", argsRow);
					return;
				}
				else if(IS_AMPERSAND(c)) {
					token[tokenIdx] = '\0';
					debug_printf("parse: token <%s> formed\n", token);
					cmdTab->args[argsRow][argsCol] = strdup(token);
					argsCol++;
					tokenIdx = 0;
					cmdTab->isbackground = true;
					currentState = AMPERSAND;
				}
				break;

			case CMD:
				if(IS_WHITESPACE(c)) {
					;
				}
				else if(IS_INPUT(c) || IS_OUTPUT(c) || IS_PIPE(c)) {
					currentState = SPECIAL;
					if(IS_INPUT(c))
						argExpected = INFILE;
					if(IS_OUTPUT(c))
						argExpected = OUTFILE;

					if(IS_PIPE(c)) {
						argsCol = 0;
						argsRow++;
						currentState = CMD;
					}
				}
				else if(IS_NORMAL(c)) {
					token[tokenIdx++] = c;
					currentState = ARGS;
				}
				else if(IS_NULL(c)) {
					argsRow++;
					cmdTab->numCmds = argsRow;
					free(token);
					debug_printf("%s\n", "parse: Exited");
					return;
				}
				else if(IS_AMPERSAND(c)) {
					cmdTab->isbackground = true;
					currentState = AMPERSAND;
				}
				break;

			case SPECIAL:
				if(IS_WHITESPACE(c)) {
					;
				}
				else if(IS_INPUT(c) || IS_OUTPUT(c) || IS_PIPE(c)) {
					printf("Parse Error: Unexpected syntax encountered.\n");
					debug_printf("%s\n", "parse: Exited");
					free(token);
					freeCmdTable(cmdTab);
					return;
				}
				else if(IS_NORMAL(c)) {
					token[tokenIdx++] = c;
					currentState = FILENAME;
				}
				else if(IS_NULL(c)) {
					printf("Parse Error: Unexpected syntax encountered.\n");
					debug_printf("%s\n", "parse: Exited");
					free(token);
					freeCmdTable(cmdTab);
					return;
				}
				else if(IS_AMPERSAND(c)) {
					cmdTab->isbackground = true;
					currentState = AMPERSAND;
				}
				break;

			case AMPERSAND:
				if(IS_NULL(c)) {
					argsRow++;
					cmdTab->numCmds = argsRow;
					free(token);
					return;
				}
				else if (IS_WHITESPACE(c)) {
					;
				}
				else {
					printf("Parse Error: Unexpected Syntax Error encountered\n");
					debug_printf("%s\n", "parse: Exited");
					free(token);
					freeCmdTable(cmdTab);
					return;
				}
				break;

			case FILENAME:
				if(IS_NORMAL(c)) {
					token[tokenIdx++] = c;
				}
				else {
					if(argExpected == INFILE) {
						token[tokenIdx] = '\0';
						debug_printf("parse: infile <%s> formed\n", token);
						free(cmdTab->infile);
						cmdTab->infile = strdup(token);
						tokenIdx = 0;
					}
					else if(argExpected == OUTFILE) {
						token[tokenIdx] = '\0';
						debug_printf("parse: outfile <%s> formed\n", token);
						free(cmdTab->outfile);
						cmdTab->outfile = strdup(token);
						tokenIdx = 0;
					}
					
					argExpected = COMMAND;

					if(IS_WHITESPACE(c)) {
						currentState = CMD;
					}
					else if(IS_INPUT(c)) {
						argExpected = INFILE;
						currentState = SPECIAL;
					}
					else if(IS_OUTPUT(c)) {
						argExpected = OUTFILE;
						currentState = SPECIAL;
					}
					else if(IS_PIPE(c)) {
						argsRow++;
						argsCol = 0;
						currentState = CMD;
					}
					else if(IS_AMPERSAND(c)) {
						cmdTab->isbackground = true;
						currentState = AMPERSAND;
					}
					else if(IS_NULL(c)) {
						argsRow++;
						cmdTab->numCmds = argsRow;
						free(token);
						debug_printf("%s\n", "parse: Exited");
						return;
					}
				}
				break;
		}
		i++;
	}

	debug_printf("%s\n", "parse: Exited");
}

void printCmdTable(cmdTable *cmdTab) {
	printf("===============================================\n");
	printf("Command Table:\n");
	printf("Command Line: %s\n", cmdTab->cmdLine);	
	printf("Number of commands: %d\n", cmdTab->numCmds);
	int argsRow, argsCol;
	for(argsRow = 0; argsRow < cmdTab->numCmds; argsRow++) {
		printf("Command -> ");
		argsCol = 0;
		while(cmdTab->args[argsRow][argsCol]) {
			printf("<%s> ", cmdTab->args[argsRow][argsCol]);
			argsCol++;
		}
		printf("\n");
	}
	printf("Input file: %s\n", cmdTab->infile);
	printf("Output file: %s\n", cmdTab->outfile);
	printf("Is background? %s\n", cmdTab->isbackground?"true":"false");
	printf("===============================================\n");
}

/* int main(int argc, char **argv) {
	cmdTable * cmdTab = calloc(1, sizeof(cmdTable));
	char cmdLine[] = "sfha;kjs |s ssdno | sdfjb -sdf -dsf a < asf askjb > asbduob | asfob";
	
	initCmdTable(cmdTab);
	parse(cmdLine, cmdTab);
	printCmdTable(cmdTab);
	freeCmdTable(cmdTab);

	free(cmdTab);
	return 0;
} */