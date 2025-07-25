
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%x src

%{

#include <cstring>
#include <string>
#include "y.tab.hh"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "shell.hh"
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS; 
}

"|" {
  return PIPE;
}

"&" {
  return AMPERSAND; 
}

"2>>" {
  return TWOGREATGREAT;
}

">>&" {
  return GREATGREATAMPERSAND; 
}
">>" {
  return GREATGREAT;
}
"2>" {
  return TWOGREAT; 
}
">&" {
  return GREATAMPERSAND;
}


\"([^\\"]|\\.)*\"   {
  // Double-quoted string, strip the quotes
  std::string *s = new std::string(yytext + 1, yyleng - 2);
  yylval.cpp_string = s;
  return WORD;
}

\'([^\\']|\\.)*\'   {
  // Single-quoted string, strip the quotes
  std::string *s = new std::string(yytext + 1, yyleng - 2);
  yylval.cpp_string = s;
  return WORD;
}

([^ \\\t\n]*(\\[^nt])((\\[^nt])|[^ \\\t\n])*) {
  std::string str = std::string(yytext);
  std::size_t found = str.find('\\');
  while (found != std::string::npos) {
    str = str.substr(0, found) + str.substr(found + 1);
    found = str.find('\\', found + 1);
  }
  yylval.cpp_string = new std::string(str);
  return WORD;
}

source {
  BEGIN(src);
}

/* <src>[^ \t\n][^ \t\n]* {
  FILE * in = fopen(yytext, "r+");

  if (!in) {
    perror("fopen");
    BEGIN(INITIAL);
  } else {
    fputc('\n', in);
    yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE));
    BEGIN(INITIAL);
    Shell::_srcCmd = true;
    yyparse();
    yypop_buffer_state();
    fclose(in);
    Shell::_srcCmd = false;
  }
}*/

<src>[^ \t\n]+ {
  FILE *in = fopen(yytext, "r");
  if (!in) {
    perror("fopen");
    BEGIN(INITIAL);
    return NEWLINE;  // prevent further parsing
  }

  Shell::_srcCmd = true;
  yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE));
  yyparse();
  yypop_buffer_state();
  fclose(in);
  Shell::_srcCmd = false;

  BEGIN(INITIAL);
  return NEWLINE;  // prevent fallthrough
}


[^ \|>\t\n][^ \|>\t\n]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

<<EOF>> {
  // Exit cleanly on EOF
  exit(0);
}


.            { /* ignore unknown character */ }

%%
