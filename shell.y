
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

command: simple_command
       ;

simple_command:	
  command_and_args iomodifier_opt background_opt NEWLINE {
    printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE 
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  | command_and_args PIPE command_word argument_list {
    printf("   Yacc: pipe to next command\n");
    Shell::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand->insertArgument( $1 );\
  }
  ;

command_word:
  WORD {
    printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

iomodifier_opt:
  iomodifier_opt io_modifier
  | /* can be empty */ 
  ;

io_modifier:
    GREAT WORD {
      printf("   Yacc: insert output \"%s\"\n", $2->c_str());
      Shell::_currentCommand._outFile = $2;
    }
  | GREATGREAT WORD {
      printf("   Yacc: insert output append \"%s\"\n", $2->c_str());
      Shell::_currentCommand._outFile = $2;
    }
  | GREATAMPERSAND WORD {
      printf("   Yacc: insert stdout+stderr \"%s\"\n", $2->c_str());
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = $2;
    }
  | GREATGREATAMPERSAND WORD {
      printf("   Yacc: insert append stdout+stderr \"%s\"\n", $2->c_str());
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = $2;
    }
  | LESS WORD {
      printf("   Yacc: insert input \"%s\"\n", $2->c_str());
      Shell::_currentCommand._inFile = $2;
    }
  | TWOGREAT WORD {
      printf("   Yacc: insert stderr \"%s\"\n", $2->c_str());
      Shell::_currentCommand._errFile = $2;
    }
  ;

background_opt:
    AMPERSAND {
      printf("   Yacc: Background flag set\n");
      Shell::_currentCommand._background = true;
    }
  | /* empty */
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
