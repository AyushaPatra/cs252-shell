#include <cstdio>

#include "shell.hh"
#include <unistd.h>

int yyparse(void);

void Shell::prompt() {
  printf("myshell>");
  if (isatty(0) ) {
    printf("myshell>");
    fflush(stdout);
  }
}

int main() {
  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
