#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <linux/limits.h>

#include "../includes/jobs_control.h"
#include "../includes/fonctions.h"


int main(int argc, char** argv)
{
	//////////////////////////////////////// VARIABLES ////////////////////////////////////////////////

	// Prompt
	const int HOST_NAME_MAX = 64;
	char hostname[HOST_NAME_MAX];
	// Get command line
	char *line = NULL, *line_cpy_guill = NULL, *line_cpy_pipe = NULL;
	int line_sz = 0;
	char *save_pipe, *token;
	process *p, *pLast;
	// Command line in args
	char **args_array = NULL;
	int ntok_read;
	// Redirection
	char *ifile, *ofile, *errfile;
	// Internal Command
	bool cmd_cd = false, cmd_fg = false, cmd_bg = false;

	//////////////////////////////////////// FONCTIONNEMENT ////////////////////////////////////////////

	// Init Shell
	init_shell();
	first_job = NULL;
	stopped_now = NULL;

	// Core of the shell functionning --------------------------------------------------------------------
	while (true) 
	{
		// Create and display prompt
		gethostname(hostname, HOST_NAME_MAX);
		printf("%s@%s:%s> ", getenv("USER"), hostname, getenv("PWD"));

		// Read a line (until \n)
		line = getCmd(&line_sz);
		line_cpy_guill = NULL;

		// Internal Command Check --------------------------------------------------------------------
		// Exit
		if (!strcmp(line,"exit"))
		{
			if(first_job != NULL) {
				job *i;
				for(i = first_job; i != NULL; i = i->next)
					free_job(i);
			}
			free(line);
			break;
		}
		if(strlen(line) >= 2) {
			cmd_cd = (line[0] == 'c' && line[1] == 'd');
			cmd_fg = (line[0] == 'f' && line[1] == 'g');
			cmd_bg = (line[0] == 'b' && line[1] == 'g');
		}
		// Check others internal commands (cd, fg and bg)
		if(cmd_cd || cmd_fg || cmd_bg) {
			line_cpy_guill = malloc(sizeof(char) * (strlen(line) + 1));
			strcpy(line_cpy_guill, line);
			args_array = parsing(line, line_cpy_guill, &ntok_read, &ifile, &ofile, &errfile);
			// CD
			if(cmd_cd) {
				if(args_array[1] == NULL) {
					if(chdir(getenv("HOME")) == -1)
						printf("Repository %s doesn't exist\n", args_array[1]);
				}
				else {
					if(chdir(args_array[1]) == -1)
						printf("Repository %s doesn't exist\n", args_array[1]);
				}

				// Update PWD
				char absol_path[PATH_MAX];
				realpath("./", absol_path);
				setenv("PWD", absol_path, 1);
			}
			// FG
			else if(cmd_fg) {
				if(args_array[1] == NULL && stopped_now != NULL)
					continue_job(stopped_now, 1);
				else if(args_array[1] != NULL) {
					job *tmp = find_job(atoi(args_array[1]));
					if(tmp != NULL && tmp->first_process->stopped) // SI existe et est réellement stoppé
						continue_job(tmp, 1);
				}
			}
			// BG
			else {
				if(args_array[1] == NULL && stopped_now != NULL)
					continue_job(stopped_now, 0);
				else if(args_array[1] != NULL) {
					job *tmp = find_job(atoi(args_array[1]));
					if(tmp != NULL && tmp->first_process->stopped) // SI existe et est réellement stoppé
						continue_job(tmp, 0);
				}
			}

			cmd_cd = false;
			cmd_fg = false;
			cmd_bg = false;

			free(line);
			free(line_cpy_guill);
			free(args_array);
		}
		// Not a internal command -----------------------------------------------------------------------
		else {
			// Create a new job
			job *j = malloc(sizeof(job));
			j->next = first_job;
			j->command = line;
			j->first_process = NULL;
			pLast = NULL;
			j->pgid = 0;
			j->stdin = fileno(stdin);
			j->stdout = fileno(stdout);
			j->stderr = fileno(stderr);

			// PIPE
			// Init for strtok
			line_cpy_pipe = malloc(sizeof(char) * (line_sz + 1));
			strcpy(line_cpy_pipe, line);
			token = strtok_r(line_cpy_pipe, "|", &save_pipe);
			
			// Create sub-process
			while(token != NULL) {
				// Parse line into tokens
				line_cpy_guill = malloc(sizeof(char) * (strlen(token) + 1));
				strcpy(line_cpy_guill, token);
				args_array = parsing(token, line_cpy_guill, &ntok_read, &ifile, &ofile, &errfile);

				// Create the process
				p = malloc(sizeof(process));
				p->argv = args_array;
				p->next = NULL;
				if(pLast == NULL)
					j->first_process = p;
				else
					pLast->next = p;
				pLast = p;

				// Prepare redirection of child
				int i_desc = -1, o_desc = -1, err_desc = -1;
				if (ifile) {
					i_desc = open(ifile, O_RDONLY);
					j->stdin = i_desc;
				}
				if (ofile) {
					o_desc = open(ofile, O_CREAT | O_WRONLY, 0644);
					j->stdout = o_desc;
				}
				if(errfile) {
					err_desc = open(errfile, O_CREAT | O_WRONLY);
					j->stderr = err_desc;
				}

				token = strtok_r(NULL, "|", &save_pipe);
			}

			// Add new job to our list
			first_job = j;

			// Launch the job and command
			if(line[line_sz - 1] == '&')
				launch_job(j, 0);
			else
				launch_job(j, 1);
		}

		// Check for finished jobs
		do_job_notification();
	}

	return 0;
}

