#include <cstdio>
#include "shell.hh"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "y.tab.hh"

int yyparse(void);

void Shell::prompt() {
  if ( isatty(0) & !_srcCmd ){
    char * pPrompt = getenv("PROMPT");
    if (pPrompt != NULL) printf("%s ", pPrompt);
  	else printf("myshell>");
    fflush(stdout);
  }
}

extern "C" void sigIntHandler(int sig)
{
  if (sig == SIGINT) {
  	Shell::_currentCommand.clear();
  	printf("\n");
  	Shell::prompt();
  }

  if (sig == SIGCHLD) {
  	pid_t pid = waitpid(-1, NULL, WNOHANG);
  	for (unsigned i=0; i<Shell::_bgPIDs.size(); i++) {
  		if (pid == Shell::_bgPIDs[i]) {
  			printf("[%d] exited\n", pid);
  			Shell::_bgPIDs.erase(Shell::_bgPIDs.begin()+i);
  			break;
  		}
  	}
  }
}


int main() {

  struct sigaction sa;
  sa.sa_handler = sigIntHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  Shell::prompt();

  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }
  if (sigaction(SIGCHLD, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }

  yyparse();
}

Command Shell::_currentCommand;
std::vector<int> Shell::_bgPIDs;