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
void sigintHandler(int sig) {
  if (sig == SIGINT) {
    write(1, "\n", 1);
    Shell::_currentCommand.clear();
    Shell::prompt();
    fflush(stdout);
  }
}


// Signal handler for SIGCHLD (Zombie cleanup)
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

  // Re-display the prompt if interactive
  /*if (isatty(0)) {
    Shell::prompt();
  }*/
}


int main() {

  // Set up SIGINT handler (Ctrl-C)

  struct sigaction sa;
  sa.sa_handler = sigintHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa, NULL);

  // Set up SIGCHLD handler (Zombie cleanup)
  struct sigaction sa_chld;
  sa_chld.sa_handler = sigchldHandler;
  sigemptyset(&sa_chld.sa_mask);
  sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa_chld, NULL);

  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
