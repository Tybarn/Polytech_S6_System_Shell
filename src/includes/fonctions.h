#ifndef FONCTION_H_
#define FONCTION_H_

char* concat(char* str1, char* str2);
char** parsing(char *line, char *line_cpy, int *ntok_read, char **ifile, char **ofile, char **errfile);
char* getCmd(int *line_szS);

#endif