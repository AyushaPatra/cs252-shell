
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <string>
#include "y.tab.hh"

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

/*
\"([^\\"]|\\.)*\" {
  std::string *s = new std::string(yytext + 1, yyleng - 2);
  yylval.cpp_string = s;
  return WORD;
}

\'([^\\']|\\.)*\' {
  std::string *s = new std::string(yytext + 1, yyleng - 2);
  yylval.cpp_string = s;
  return WORD;
}

\\[^\n] {
  std::string *s = new std::string(1, yytext[1]);
  yylval.cpp_string = s;
  return WORD;
}
*/

([^\ \t\n\"\'\\><|&;()]*\\[^\n])([^\ \t\n\"\'\\><|&;()]*)+ {
  std::string *s = new std::string();

  for (int i = 0; i < yyleng; i++) {
    if (yytext[i] == '\\' && i + 1 < yyleng) {
      i++;  // skip the backslash
      s->push_back(yytext[i]);  // add the escaped char literally
    } else {
      s->push_back(yytext[i]);
    }
  }

  yylval.cpp_string = s;
  return WORD;
}


[^ \t\n][^ \t\n]*  {
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
