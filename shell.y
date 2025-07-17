
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
%token GREAT GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND
%token LESS TWOGREAT TWOGREATGREAT
%token PIPE AMPERSAND
%token NEWLINE NOTOKEN

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: 
  pipe_list iomodifier_list background_opt NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | error NEWLINE { yyerrok; }
  ;

simple_command:	
  command_and_args 
  ;

command_and_args:
  command_word argument_list {    
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    //Command::_currentSimpleCommand->insertArgument( $1 );
    wildCard = false;
    char *p = (char *)"";
    expandWildcard(p, (char *)$1->c_str());
    std::sort(_sortArgument.begin(), _sortArgument.end(), cmpfunction);
    for (auto a: _sortArgument) {
      std::string * argToInsert = new std::string(a);
      Command::_currentSimpleCommand->insertArgument(argToInsert);
    }
    _sortArgument.clear();
    //if (!wildCard) Command::_currentSimpleCommand->insertArgument($1);
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

pipe_list:
  pipe_list PIPE simple_command 
  | simple_command 
  ;

iomodifier_opt:
  GREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);
  }
  | GREATGREAT WORD {
    //printf("   Yacc: append output \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);
    Shell::_currentCommand._append = true;
  }
  | GREATAND WORD {
    //printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);
    Shell::_currentCommand.redirect(2, new std::string($2->c_str()));
    
  }
  | GREATGREATAND WORD {
    //printf("   Yacc: append output and error \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);
    Shell::_currentCommand.redirect(2, new std::string($2->c_str()));
    Shell::_currentCommand._append = true;
  }
  | LESS WORD {
    //printf("   Yacc: get input \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(0, $2);
  }
  | TWOGREAT WORD {
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(2, $2);
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  | /*empty*/
  ;

background_opt:
  AMPERSAND {
    //printf("   Yacc: run in background\n");
    Shell::_currentCommand._background = true;
  }
  | /*empty*/
  ;

%%

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
