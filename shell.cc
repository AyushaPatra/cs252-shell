#include <cstdio>
#include "command.hh"
#include "shell.hh"
#include <unistd.h>
#include <signal.h>
#include "y.tab.hh"

int yyparse(void);

void Shell::prompt() {
  if (isatty(0)) {
  	printf("myshell>");
  }
  fflush(stdout);
}
extern "C" void disp( int sig )
{
	//if (sig == SIGINT) {
                //if (isatty(0)) {
			printf("^C");
                        printf("\n");
			Shell::_currentCommand.clear();
			Shell::prompt();
                //}
        //}
}
int main() {
  //Shell::prompt();

    struct sigaction sa;
    sa.sa_handler = disp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sa, NULL)){
        perror("sigaction");
        exit(2);
    }

    Shell::prompt();
    yyparse();
}

Command Shell::_currentCommand;
