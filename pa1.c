/**********************************************************************
 * Copyright (c) 2021-2022
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"

/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
extern struct list_head history;

struct entry {
		struct list_head list;
		char *string;
};

int timeout = 2;
int child_pid;
char * name;

void alarm_handler(int signal) {
	if (signal == SIGALRM) {
		kill(child_pid, SIGKILL);
	}
	fprintf(stderr, "%s is timed out\n", name);
}


/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
int run_command(int nr_tokens, char * const tokens[])
{	
	name = (char*)malloc(sizeof(char)*strlen(tokens[0]));
	strcpy(name, tokens[0]);

	if (strcmp(tokens[0], "exit") == 0) return 0;

	else if (strcmp(tokens[0], "cd") == 0) {
		if (nr_tokens==1) {
			chdir(getenv("HOME"));
		}
		else {
			if (strcmp(tokens[1], "~") == 0) {
				chdir(getenv("HOME"));
			}
			else {
				chdir(tokens[1]);
			}
		}
		return 1;
	}

	else if (strcmp(tokens[0], "history") == 0 ){
		int index=0;
		struct entry *new = malloc(sizeof(struct entry));

		list_for_each_entry(new, &history, list)
		{
			fprintf(stderr, "%2d: %s", index, new->string);
			index++;
		};
		return 1;
	}
	
	else if (strcmp(tokens[0], "!") == 0){
		if (nr_tokens == 1 || nr_tokens > 2) return 0;
		else {
			int process_command(char * command);
			struct entry *order = malloc(sizeof(struct entry));
			int index_num = atoi(tokens[1]);
			int num = 0;
			list_for_each_entry(order, &history, list){
				if (num == index_num) {
					strcpy(tokens[0], order->string);
					process_command(tokens[0]);
				}
				num++;
			}
			return 1;
		}
	}

    else if (strcmp(tokens[0], "timeout") == 0){
        if (nr_tokens == 1) fprintf(stderr, "Current timeout is 0 second\n");
        else if (atoi(tokens[1]) == 0) {
			timeout = 0;
			fprintf(stderr, "Timeout is disabled\n");
		}
        else {
			timeout = atoi(tokens[1]);
            fprintf(stderr, "Timeout is set to %d seconds\n", atoi(tokens[1]));
        }
        return 1;
    }	

	struct sigaction signal;
    signal.sa_handler = &alarm_handler;

	sigaction(SIGALRM, &signal, 0);
	alarm(timeout);

	pid_t pid;
	pid = fork();

	if (pid > 0) {
		child_pid = pid;
		waitpid(pid, &nr_tokens, 0);
	}
	
	else if (execvp(tokens[0], tokens) < 0) {
		fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		exit(0);
	}

	else return -EINVAL;
	return 1;
}


/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
void append_history(char * const command)
{
	struct entry *new = malloc(sizeof(struct entry));
	new->string = (char*)malloc(sizeof(char)*strlen(command) + 1);
	strcpy(new->string, command);
	new->list = history;

	list_add_tail(&(new->list), &history);
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char * const argv[])
{

}


/***********************************************************************
 * process_command(command)
 *
 * DESCRIPTION
 *   Process @command as instructed.
 */
int process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}
