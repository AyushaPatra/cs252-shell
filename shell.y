
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include<regex.h>
#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE LESS GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND PIPE AMPERSAND TWOGREAT SOURCE

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

#include <string.h>
int cmp(const void *a, const void *b);
void expandWildcardsIfNecessary(char * arg);
void expandWildcard(char * prefix, char *suffix);
void yyerror(const char * s);
int yylex();
%}

%%

goal: command_list;


arg_list:
	arg_list WORD {
    	  //printf("   Yacc: insert argument \"%s\"\n", $2->c_str());
	  //Command::_currentSimpleCommand->insertArgument( $2 );
	  expandWildcardsIfNecessary((char*)($2->c_str()));
  	}
	| /*empty string*/
	;

cmd_and_args:
	WORD {
   	  //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    	  if (strcmp($1->c_str(), "exit") == 0) {
                if (isatty(0)) {
			printf("\nGood bye!!\n\n");
		}
                exit(0);
          }
	  Command::_currentSimpleCommand = new SimpleCommand();
	  Command::_currentSimpleCommand->insertArgument($1);
  	}  arg_list {
   	  Shell::_currentCommand.
    	  insertSimpleCommand( Command::_currentSimpleCommand );
  	}
	;

pipe_list:
	pipe_list PIPE cmd_and_args
	| cmd_and_args
	;

io_modifier:
	GREATGREAT WORD {
          //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
          if (Shell::_currentCommand._outFile != NULL) {
                yyerror("Ambiguous output redirect.\n");
          }
	  Shell::_currentCommand._outFile = $2;
	  Shell::_currentCommand._append = true;
        }
	| GREAT WORD {
    	  //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    	  if (Shell::_currentCommand._outFile != NULL) {
		yyerror("Ambiguous output redirect.\n");
	  }
	  Shell::_currentCommand._outFile = $2;
  	}
	| GREATGREATAMPERSAND WORD {
          //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
	  if (Shell::_currentCommand._outFile != NULL | Shell::_currentCommand._errFile != NULL) {
                yyerror("Ambiguous output redirect.\n");
          }
          Shell::_currentCommand._outFile = $2;
	  Shell::_currentCommand._errFile = $2;
	  Shell::_currentCommand._append = true;
        }
	| GREATAMPERSAND WORD {
          //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
          if (Shell::_currentCommand._outFile != NULL | Shell::_currentCommand._errFile != NULL) {
                yyerror("Ambiguous output redirect.\n");
          }
	  Shell::_currentCommand._outFile = $2;
	  Shell::_currentCommand._errFile = $2;
        }
	| LESS WORD {
          //printf("   Yacc: insert input \"%s\"\n", $2->c_str());
          if (Shell::_currentCommand._inFile != NULL) {
                yyerror("Ambiguous output redirect.\n");
          }
	  Shell::_currentCommand._inFile = $2;
	}
	| TWOGREAT WORD {
	  //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
          if (Shell::_currentCommand._errFile != NULL) {
                yyerror("Ambiguous output redirect.\n");
          }
	  Shell::_currentCommand._errFile = $2;
	}
	;

io_modifier_list:
	io_modifier_list io_modifier
	| /*empty*/
	;

background_optional:
	AMPERSAND {
          /*printf("   Yacc: insert output \"%s\"\n", $1->c_str());*/
          Shell::_currentCommand._background = true;
        }
	| /*empty*/
	;

command_line:
	pipe_list io_modifier_list
	background_optional NEWLINE {
    	  //printf("   Yacc: Execute command\n");
    	  Shell::_currentCommand.execute();
  	}
	| NEWLINE {
          Shell::_currentCommand.execute();
        } /*accept empty cmd line*/
	| error NEWLINE{yyerrok;}
	; /*error recovery*/

command_list :
	command_line |
	command_list command_line
	;/* command loop*/


%%
int maxEntries;
int nEntries;
char ** array;
bool flag = false;
#define MAXFILENAME 1024
void expandWildcard(char * prefix, char *suffix) {
	if (suffix[0]== 0) {

		if (nEntries == maxEntries) {
			maxEntries *=2;
                        array = (char**)realloc(array, maxEntries*sizeof(char*));
  	      }

		//array[nEntries] = (char*)malloc(strlen(prefix)+10);
		prefix++;
//printf("final prefix is %s\n", prefix);
		array[nEntries]= strdup(prefix);
                //array[nEntries] = prefix;
		/*
		int m = 0;
		while (*prefix != '\0') {
			array[nEntries][m] = *prefix;
			prefix++;
			m++;
		}
		array[nEntries][m] = '\0';
		*/
		nEntries++;
		//Command::_currentSimpleCommand->insertArgument(new std::string(strdup(prefix)));
		return;
	}

	//suffix++;
	char * s = strchr(suffix, '/');
	char component[MAXFILENAME];
	//printf("suffix is %s\n", suffix);
//printf("s is %s\n", s);
	if (s!=NULL){
		int index = 0;
		component[index] = *s;
		s++;
		index++;
		if (strchr(s, '/') != NULL) {
			while (*s != '/') {
				component[index] = *s;
				s++;
				index++;
			}
		} 
		else {
			while (*s != '\0') {
                                component[index] = *s;
                                s++;
                                index++;
                        }
		}
		component[index] = '\0';
//printf("componet is %s\n", component);
		//printf("s is %s\n", s);
		suffix = s;
	}
	else {
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}
	char newPrefix[MAXFILENAME];
	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
		//sprintf(newPrefix,"%s/%s", prefix, component);
		int i = 0;
		int j = 0;
		if (prefix != NULL) {
			while (*prefix != '\0') {
				newPrefix[i] = *prefix;
				i++;
				prefix++;
			}
		}
		while (component[j] != '\0') {
			newPrefix[i] = component[j];
			i++;
			j++;
		}
		newPrefix[i] = '\0';
//printf("newPrefix is %s\n", newPrefix);
//printf("suffix is %s\n", suffix);
		expandWildcard(newPrefix, suffix);
		return;
	}

	char * reg = (char*)malloc(2*strlen(component)+10);
        char * a = component;
        char * r = reg;
        *r = '^';
        r++;
	a++;
        while (*a) {
		if (*a == '*') { *r='.'; r++; *r='*'; r++; }
                else if (*a == '?') { *r='.'; r++;}
                else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
                else { *r=*a; r++;}
                a++;
        }
        *r='$';
        r++;
        *r=0;
//printf("reg is %s\n", reg);	
	regex_t re;
	int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
	char * dir;
	if (prefix == NULL) {
		//strcpy(dir, ".");
		dir = (char*)".";
		//dir++;
		
	}
	else {
		dir = prefix;
	}

	DIR * d = opendir(dir);
	if (d==NULL) {
		return;
	}
//printf("dir is %s\n", dir);
	struct dirent * ent;
	regmatch_t match;
	while ((ent = readdir(d))!= NULL) {
		if (regexec(&re, ent->d_name, 1, &match, 0) ==0) {
			//printf("find match\n");
			if (ent->d_name[0] == '.') {
                        	if (flag) {
                                	sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
                                	expandWildcard(newPrefix,suffix);
                        	}
                	}
			else {
				sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
				//printf("newPrefix is %s\n", newPrefix);
				//printf("newsuffix is %s\n", suffix);
				expandWildcard(newPrefix,suffix);
			}
		}
	}
	regfree(&re);
	free(reg);
	closedir(d);
}// expandWildcard


int cmp(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}
void expandWildcardsIfNecessary(char * arg)
{
	
	if (strchr(arg, '*') == NULL && strchr(arg, '?') == NULL) {
		//printf("no wildcard\n");
		//printf("arg is %s\n", arg);
		Command::_currentSimpleCommand->insertArgument(new std::string(arg));
		return;
	}
	else if (strchr(arg, '/') == NULL) {
		//printf("wildcar case1\n");
		//printf("arg is %s\n", arg);
		char * reg = (char*)malloc(2*strlen(arg)+10);
		char * a = arg;
		char * r = reg;
		*r = '^';
		r++;
		while (*a) {
			if (*a == '*') { *r='.'; r++; *r='*'; r++; }
			else if (*a == '?') { *r='.'; r++;}
			else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
			else { *r=*a; r++;}
			a++;
		}
		*r='$';
		r++;
		*r=0;

		regex_t re;

		int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
//printf("reg is %s\n", reg);
		DIR * dir = opendir(".");
		if (dir == NULL) {
			perror("opendir");
			return;
		}

		regmatch_t match;
		struct dirent * ent;
		maxEntries = 20;
		nEntries = 0;
		array = (char**) malloc(maxEntries*sizeof(char*));

		while ( (ent = readdir(dir))!= NULL) {
			if (regexec(&re, ent->d_name, 1, &match, 0) ==0 ) {
				if (nEntries == maxEntries) {
					maxEntries *=2;
					array = (char**)realloc(array, maxEntries*sizeof(char*));
				}

				if (ent->d_name[0] == '.') {
					if (arg[0] == '.') {
						array[nEntries]= strdup(ent->d_name);
						nEntries++;
					}
				}
				else {
					array[nEntries]= strdup(ent->d_name);
                                        nEntries++;
				}
				//Command::_currentSimpleCommand->insertArgument(new std::string(strdup(ent->d_name)));

			}
		}
		regfree(&re);
		free(reg);
		closedir(dir);
		//sortArrayStrings(array, nEntries);
	}
	else {
		//printf("wildcard case2\n");
		maxEntries = 20;
                nEntries = 0;
                array = (char**) malloc(maxEntries*sizeof(char*));

		if (arg[0] == '.') {
			flag = true;
		}
		expandWildcard((char*)"/", arg); 
	}
//printf("successfully get array\n");
	//char temp[5000];
//printf("nEntries is %d\n", nEntries);
	
	qsort(array, nEntries, sizeof(char *), cmp);
/*
        for (int i = 0; i < nEntries-1; i++) {
                for (int j = i+1; j < nEntries; j++) {
                        if (strcmp(array[i], array[j]) > 0) {
                                strcpy(temp, array[i]);
                                strcpy(array[i], array[j]);
                                strcpy(array[j], temp);
                        }
                }
        }
*/
//printf("sort success\n");

	if (array[0] == NULL) {
		Command::_currentSimpleCommand->insertArgument(new std::string(arg));
	}
        for (int i = 0; i < nEntries; i++) {
                //printf("array[i] is %s\n", array[i]);
		Command::_currentSimpleCommand->insertArgument(new std::string(array[i]));

        }
//printf("insert arg success\n");
	for (int i = 0; i < nEntries; i++) {
		free(array[i]);
	}
        free(array);
}

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}
#if 0
main()
{
  yyparse();
}
#endif
