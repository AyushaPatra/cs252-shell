
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <limits.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "shell.hh"
#include "y.tab.hh"
////////////  Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
  // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}
/*
void source(char * file) {
	FILE* fd = fopen(file, "r");
	YY_BUFFER_STATE buf = yy_create_buffer(fd, 100);
	yypush_buffer_state(buf);
	yyparse();
	yypop_buffer_state();
	fclose(fd);
	yyrestart(stdin);
}
*/
%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

"|" {
  return PIPE;
}

"&" {
  return AMPERSAND;
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}

"2>" {
  return TWOGREAT;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

["][^\n\"]*["] { 
	char * temp = yytext+1;
	while (*temp != '\"') {
		temp++;
	}
	*temp = '\0';
	yylval.cpp_string = new std::string(yytext+1);

	return WORD;
} 

[^ \t\n|><&]*\\[^ \t\n]* {
	char * arg = (char *)malloc(200);
	char * temp = yytext;
	char * a = arg;

	while(*temp){
		if (*temp == '\\'){
			temp++;
			*a = *temp;
			a++;
		} else {
			*a = *temp;
			a++;
		}
		temp++;
	}
	*a = '\0';

	char final[100];
	strcpy(final, arg);
	free(arg);
	yylval.cpp_string = new std::string(final);
	return WORD;
}

[~][^ \n]* {
	char * temp = yytext;
	if (strlen(temp) == 1) {
		char * out = getenv("HOME");
        	yylval.cpp_string = new std::string(out);
        	return WORD;
	}
	else {
		char out[100];
		strcpy (out, "/homes/");
		temp++;
		strcat(out, temp);
                yylval.cpp_string = new std::string(out);
                return WORD;
	}
}

[$(][^\n\$]*[)]|`[^\n`]*` {
	//printf("subshell\n");
	char * temp = yytext;
	//printf("temp is %s\n", temp);

	char * cmd = (char *)malloc(200);
	char * c = cmd;
	if (strchr(temp, '\'') != NULL) {
		temp++;
		while (*temp != '\'') {
			*c = *temp;
			c++;
			temp++;
		}
		*c = '\0';
			
	}
	else {
		temp = temp + 2;
                while (*temp != ')') { 
                        *c = *temp;
			c++;
			temp++;
                }
                *c = '\0';
			
	}
	//printf("after cmd is %s\n", cmd);
	//printf("len cmd is %ld\n", strlen(cmd));
	
	int tmpin = dup(0);
	int tmpout = dup(1);

	int fdpipein[2];
	int fdpipeout[2];

	pipe(fdpipein);
	pipe(fdpipeout);

	write(fdpipein[1], cmd, strlen(cmd));
	write(fdpipein[1], "\n", 1);
	write(fdpipein[1], "exit", 4);
	write(fdpipein[1], "\n", 1);
	close(fdpipein[1]);

	dup2(fdpipein[0], 0);
	close(fdpipein[0]);
	dup2(fdpipeout[1], 1);
	close(fdpipeout[1]);

	int ret = fork();
	if (ret == 0) {
		execvp("/proc/self/exe", NULL);
		perror("execvp");
		exit(1);
	}

	dup2(tmpin, 0);
	dup2(tmpout, 1);
	close(tmpin);
	close(tmpout);

	char ch;
	char * buf1 = new char[5000];
	char * buf2 = buf1;
	while (read(fdpipeout[0], &ch, 1)) {
		if ( ch == '\n') {
			 *buf2 = ' ';
		}
		else {
			 *buf2 = ch;
		}
		buf2++;
	}
	*buf2 = '\0';


	int index = strlen(buf1)-1;
	for (int i = index; i >= 0; i--) {
		myunputc(buf1[i]);
	}
}

[^ \t\n|><&][^ \t\n|><&]*  {
  /* Assume that file names have only alpha chars */
  
	
	char * temp = yytext;
        char * cmd = (char *)malloc(2000);
        char * c = cmd;

	while (*temp != '\0') {
		if ((*temp == '$') && (*(temp+1) == '{')) {
			char * str = (char*)malloc(100);
			char * str1 = str;
			temp = temp + 2;
			while (*temp != '}') {
				*str1 = *temp;
				str1++;
				temp++;
			}
			*str1 = '\0';
			temp++;
			if (strcmp(str, "$") == 0) {
				std::string * s = new std::string(std::to_string(getpid()));
                		char * str2 = (char *)s->c_str();
                		while (*str2 != '\0') {
					*c = *str2;
					c++;
					str2++;
				}
        		}
        		else if (strcmp(str, "?") == 0) {
				//printf("return value is %d\n", Shell::_currentCommand._lastRT);
				std::string * s = new std::string(std::to_string(Shell::_currentCommand._lastRT));
                                char * str2 = (char *)s->c_str();
                                while (*str2 != '\0') {
                                        *c = *str2;
                                        c++;
                                        str2++;
                                }
        		}
        		else if (strcmp(str, "!") == 0) {
				//printf("PID is %d\n", Shell::_currentCommand._lastPID);
				std::string * s = new std::string(std::to_string(Shell::_currentCommand._lastPID));
                                char * str2 = (char *)s->c_str();
				//printf("PID is %s\n", str2);
                                while (*str2 != '\0') {
                                        *c = *str2;
                                        c++;
                                        str2++;
                                }
        		}
        		else if (strcmp(str, "_") == 0) {
				std::string * s = new std::string(Shell::_currentCommand.getLastArg());
				char * str2 = (char *)s->c_str();
                                while (*str2 != '\0') {
                                        *c = *str2;
                                        c++;
                                        str2++;
                                }
			}
        		else if (strcmp(str, "SHELL") == 0) {
                		char path[100];
                		realpath("../shell", path);
				int index = 0;
                		while (path[index] != '\0') {
                                        *c = path[index];
                                        c++;
                                        index++;
                                }
        		}
			else {
        			char * out = getenv(str);
        			if (out != NULL) {
                			while (*out != '\0') {
						*c = *out;
						c++;
						out++;
					}
        			}
				else {
					*c = '-';
					c++;
					*c = 'n';
					c++;
				}
			}
		}
		else {
			*c = *temp;
			c++;
			temp++;
		}
	}
	*c = '\0';
//printf("final cmd is %s\n", cmd);
	char final[1000];
	strcpy(final, cmd);
	free(cmd);
  yylval.cpp_string = new std::string(final);
  //free(cmd);
  return WORD;
}

