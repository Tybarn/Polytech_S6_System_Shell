#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

// Concaténation de 2 chaines de caractères dans une nouvelle
char* concat(char* str1, char* str2) {
	int i = 0;
	char* res = (char*) malloc(sizeof(char) * (strlen(str1) + strlen(str2) + 1));

	if(res == NULL) {
		fprintf(stderr, "Concat : Allocation mémoire : Probleme lors du malloc\n");
		return NULL;
	}
	
	while(i < strlen(str1)) {
		res[i] = str1[i];
		i++;
	}
	
	int j;
	for(j = 0; j < strlen(str2); j++)
		res[i++] = str2[j];

	res[i] = '\0';
	
	return res;
}

// Get command
char* getCmd(int *line_sz) {
	const size_t BUF_SZ = 64;
	char* buffer = malloc(BUF_SZ);
	char* line = calloc(1,1); //leading zero for strcat
	*line_sz = 0;
	bool eol = false;
	
	while (true)
	{
		fgets(buffer, BUF_SZ, stdin);
		size_t buf_sz = strlen(buffer); //effective size
		if (buffer[buf_sz-1] == '\n')
		{
			if(buf_sz > 2 && buffer[buf_sz-2] == '\\') {
				buffer[buf_sz-2] = '\0';
				buf_sz -= 2;
				eol = false;
			}
			else {
				buffer[buf_sz-1] = '\0';
				buf_sz--;
				eol = true;
			}
		}
		*line_sz += buf_sz;
		line = realloc(line, *line_sz+1);
		strcat(line, buffer);
		if (eol)
			break;
	}

	free(buffer);

	return line;
}

// Parsing of the command line
char** parsing(char *line, char *line_cpy, int *ntok_read, char **ifile, char **ofile, char **errfile) {
	int args_sz = 0;
	char** args_array = NULL;
	char *save_space = NULL, *save_guill = NULL;

	*ntok_read = 0;
	*ifile = NULL;
	*ofile = NULL;
	*errfile = NULL;

	// Init strtok
	char* token = strtok_r(line, " ", &save_space);
	strtok_r(line_cpy, "\"", &save_guill);

	while (token != NULL)
	{
		// Redirections ?
		if (!strcmp(token,"<") || !strcmp(token,">"))
		{
			char redirect = token[0];
			token = strtok_r(NULL, " ", &save_space); //file name
			redirect=='<' ? (*ifile=token) : (*ofile=token);
		}
		else if(!strcmp(token,"2>"))
			*errfile = strtok_r(NULL, " ", &save_space);
		else
		{
			if (args_sz <= *ntok_read) //not enough memory
			{
				args_sz = 2*args_sz + 1; //limit the memory allocations to log(n)
				args_array = realloc(args_array, args_sz*sizeof(char*));
			}
			if(token[0] == '"') {
				while(token != NULL && token[strlen(token) - 1] != '"') // Sauter jusqu'a avoir passe les guillemets actuels
					token = strtok_r(NULL, " ", &save_space);
				token = strtok_r(NULL, "\"", &save_guill);
				strtok_r(NULL, "\"", &save_guill); // Seconde fois car le prochain saut est un espace
			}
			if(token[0] != '&')
				args_array[(*ntok_read)++] = token;
		}
		token = strtok_r(NULL, " ", &save_space);
	}
	if (args_sz <= *ntok_read)
		args_array = realloc(args_array, (args_sz+1)*sizeof(char*));
	args_array[*ntok_read] = NULL; //marker for execv to know where to end...

	return args_array;
}