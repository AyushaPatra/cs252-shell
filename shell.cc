#include <cstdio>
#include "command.hh"
#include "shell.hh"
#include <unistd.h>
#include <signal.h>
#include "y.tab.hh"

#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

void yyrestart(FILE * input_file );
int yyparse(void);
void yypush_buffer_state(YY_BUFFER_STATE buffer);
void yypop_buffer_state();
YY_BUFFER_STATE yy_create_buffer(FILE * file, int size);

int yyparse(void);

// Part 1A: Shell prompt function (displays "myshell>" when running interactively)
// This function is called after each command execution to show the user prompt
extern FILE *yyin;

void source(void) {
  std::string s = ".shellrc";
  FILE * in = fopen(s.c_str(), "r");

  if (!in) {
    return;
  }

  yyin = in;

  yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE));
  Shell::_srcCmd = true;
  yyparse();
  yypop_buffer_state();
  fclose(in);
  Shell::_srcCmd = false;
}

void Shell::prompt() {
  if (isatty(0) && !_srcCmd) {
    // Only show prompt if input is from a terminal (interactive mode)
    printf("myshell>");
  }
  fflush(stdout); // Ensure prompt is displayed immediately
}

// Part 2.9: Signal handler for Ctrl-C (SIGINT)
// When user presses Ctrl-C, clean up current command and show new prompt
extern "C" void disp( int sig )
{
  // Print ^C to show that Ctrl-C was detected (mimics bash behavior)
  printf("^C");
  printf("\n");
  
  Shell::_currentCommand.clear();
  
  Shell::prompt();
}

// Main function - entry point of the shell
int main() {
  // Part 2.9: Set up signal handling for Ctrl-C (SIGINT)
  // This allows the shell to handle Ctrl-C gracefully instead of terminating
  struct sigaction sa;
  sa.sa_handler = disp;        // Set our custom signal handler function
  sigemptyset(&sa.sa_mask);    // Don't block other signals during handler execution!
  sa.sa_flags = SA_RESTART;    // Automatically restart interrupted system calls

  

  // Install the signal handler for SIGINT (Ctrl-C)
  if(sigaction(SIGINT, &sa, NULL)){
    perror("sigaction");
    exit(2);
  }

  source();
  Shell::prompt();
  yyparse(); // Start the parser which will read and execute commands
}

// The parser and lexer use this to construct commands as they parse user input
Command Shell::_currentCommand;
bool Shell::_srcCmd;
