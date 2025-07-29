#include <cstdio>
#include "command.hh"
#include "shell.hh"
#include <unistd.h>
#include <signal.h>
#include "y.tab.hh"
#include "lex.yy.hh"

int yyparse(void);

// Part 1A: Shell prompt function (displays "myshell>" when running interactively)
// This function is called after each command execution to show the user prompt

void Shell::prompt() {
  if (isatty(0)) {
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

  // Check if ./.shellrc exists and source it
  FILE *rcFile = fopen(".shellrc", "r");
  if (rcFile) {
    fclose(rcFile);

    // Temporarily redirect stdin to the file
    FILE *in = fopen(".shellrc", "r");
    if (in) {
      extern FILE *yyin;
      yyin = in;

      // Save current input buffer and parse .shellrc
      yy_switch_to_buffer(yy_create_buffer(in, YY_BUF_SIZE));
      yyparse(); // Parse .shellrc just like input
      yy_delete_buffer(YY_CURRENT_BUFFER);
      fclose(in);
    }
  }


  Shell::prompt();
  yyparse(); // Start the parser which will read and execute commands
}

// The parser and lexer use this to construct commands as they parse user input
Command Shell::_currentCommand;
