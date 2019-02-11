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

#include "../includes/fonctions.h"
#include "../includes/jobs_control.h"

int main(int argc, char** argv) {
	//////////////////////////////////////// VARIABLES ////////////////////////////////////////////////

	// Prompt
	const int HOST_NAME_MAX = 64;
	char hostname[HOST_NAME_MAX];
	// Get command line
	char *line = NULL, *line_cpy_guill = NULL;
	int line_sz = 0;
	// Command line in args
	char **args_array = NULL;
	int ntok_read;
	// Redirection
	char *ifile, *ofile, *errfile;

	//////////////////////////////////////// FONCTIONNEMENT ////////////////////////////////////////////

	// Init Shell
	init_shell();

	// Core of the shell functionning -------------------------------------------------------
	while (true) 
	{
		// Create and display prompt
		gethostname(hostname, HOST_NAME_MAX);
		printf("%s@%s:%s> ", getenv("USER"), hostname, getenv("PWD"));

		// Read a line (until \n)
		line = getCmd(&line_sz);

		// Internal Command Check
		// Exit
		if (!strcmp(line,"exit"))
		{
			free(line);
			break;
		}

		// Get command by arguments
		args_array = parsing(line, line_cpy_guill, &ntok_read, &ifile, &ofile, &errfile);
		
		// PIPES -------------------------------------------------------------------------
        int i = 0, pipe_bool = 0, pfd[2];

        for(i = 0; i < ntok_read; i++) {
            if(!strcmp(args_array[i], "|"))
            {                                  
                pipe_bool = 1;
                i++;
                break;                    
            }
        }

	    // si pipe
	    int old_stdout,old_stdin;
	    if(pipe_bool == 1)
	    {	
		    if (pipe(pfd) == -1)
		     {
		       printf("pipe failed\n");
		       return 1;
		     }
	    	
			old_stdin = dup(0);
			old_stdout = dup(1);
	    }
	   
	    // EXECUTION ---------------------------------------------------------------------
	    if(strcmp(args_array[0], "cd"))
	    {
		    pid_t pid = fork();	
	        if (pid == 0) // fils
			{	
				int status;

				// si pipe		
			    if(pipe_bool==1)
			    {  
		 			close(pfd[0]);
					dup2(pfd[1],1);
					close(pfd[1]);
					
					int j = 0;                    
					char** args_array2 = (char**) malloc(sizeof(char*)*i);                    
					
					for(j = 0; j < i-1; j++)
					    args_array2[j] = args_array[j];

					execvp(args_array2[0], args_array2);           
			    }
			    else
			    {
					// Child: prepare redirections, execute command
					int i_desc = -1, o_desc = -1, err_desc = -1;
					if (ifile)
					{
						i_desc = open(ifile, O_RDONLY);
						dup2(i_desc, fileno(stdin)); //or STDIN_FILENO ...
					}
					if (ofile)
					{
						o_desc = open(ofile, O_CREAT | O_WRONLY, 0644);
						dup2(o_desc, fileno(stdout)); //or STDOUT_FILENO ...
					}
					if(errfile) {
						err_desc = open(errfile, O_CREAT | O_WRONLY);
						dup2(err_desc, fileno(stderr));
					}
					// Original files can be closed: they just have been duplicated
					if (ifile)
						close(i_desc);
					if (ofile)
						close(o_desc);
					if(errfile)
						close(err_desc);
					// The command is executed here
					execvp(args_array[0], args_array);
					
					// If the program arrives here, execvp failed
					fprintf(stderr, "Error while launching %s\n", args_array[0]);
					exit(EXIT_FAILURE);
			    }
			}
			else if (pid > 0) {
				// Parent: wait for child to finish
				 int status; //optional; give infos on child termination
			         // si pipe
				 //waitpid(pid, &status, 0);

				 if(pipe_bool == 1)
				 {  		              
			        close(pfd[1]);
			        dup2(pfd[0],0);
			        close(pfd[0]);  
			        waitpid(pid, &status, 0); //or just wait(&status), or wait(NULL)...
			     
			        int j = i; int y = 0;
			        char** args_array2 = (char**) malloc(sizeof(char*) * 10);               
			     
			        for(j=i;j<ntok_read;j++){
			            args_array2[y] = args_array[j];
			          	y++;
			        }
				 
				    pid_t pid2 = fork();
				 
				    if(pid2 == 0){
						execvp(args_array2[0], args_array2);
					}
					else
					{
						waitpid(pid2,&status,0);
						dup2(old_stdin,0);
						dup2(old_stdout,1);
					}

					pipe_bool = 0;			                         
			    }
			    else
			       waitpid(pid, &status, 0); //or just wait(&status), or wait(NULL)...
			    
			    if(args_array != NULL) {
					free(args_array);
					free(line);
					free(line_cpy_guill);
			    }
			}
		}
		// System functions
		else {
			// CD
			if(!strcmp(args_array[0], "cd")) {
				if(chdir(args_array[1]) == -1)
					printf("Repository %s doesn't exist\n", args_array[1]);
			}
		}
	}

	return 0;

}