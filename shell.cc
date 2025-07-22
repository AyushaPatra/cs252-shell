#include <cstdio>
#include "shell.hh"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

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

// Signal handler for SIGCHLD (Zombie processes)
void sigchldHandler(int sig) {
  int status;
  pid_t pid;

  // Getting all children that have exited
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // Safe signal output using write()
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "[%d] exited.\n", pid);
    write(1, buffer, len);
  }
}


int main() {

  struct sigaction sa;
  sa.sa_handler = ctrlCHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

  sigaction(SIGINT, &sa, NULL);

  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
