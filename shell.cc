#include <cstdio>
#include "shell.hh"
#include <unistd.h>
#include <signal.h>

int yyparse(void);

void Shell::prompt() {
  //printf("myshell>");
  if (isatty(0) ) {
    printf("myshell>");
    fflush(stdout);
  }
}

// Signal handler for SIGINT (Ctrl-C)
void ctrlCHandler(int sig) {
  write(1, "\n", 1);
  Shell::prompt();
  fflush(stdout);
}

int main() {

  struct sigaction sa;
  sa.sa_handler = ctrlCHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  sigaction(SIGINT, &sa, NULL);

  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
