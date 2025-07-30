#include <cstdio>
#include "command.hh"
#include "shell.hh"
#include <unistd.h>
#include <signal.h>
#include "y.tab.hh"

#include <string>
#include <regex>

// Helper to convert "\033" to real ESC
std::string interpret_escape_sequences(const char* raw) {
  std::string str(raw);
  str = std::regex_replace(str, std::regex(R"(\\033)"), "\033");
  str = std::regex_replace(str, std::regex(R"(\\e)"), "\033");    // Optional
  str = std::regex_replace(str, std::regex(R"(\\n)"), "\n");
  str = std::regex_replace(str, std::regex(R"(\\t)"), "\t");
  return str;
}

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
void yy_delete_buffer(YY_BUFFER_STATE buffer);

int yyparse(void);

// Part 1A: Shell prompt function (displays "myshell>" when running interactively)
// This function is called after each command execution to show the user prompt
extern FILE *yyin;

extern "C" void tty_restore_mode(void);

void Shell::prompt() {
  if (isatty(0) && !_srcCmd) {
    // Only show prompt if input is from a terminal (interactive mode)
    //printf("myshell>");
    const char* raw = getenv("PS1");
    if (raw) {
      std::string promptStr = interpret_escape_sequences(raw);
      printf("%s", promptStr.c_str());
    } else {
      printf("myshell> ");


  }
  fflush(stdout); // Ensure prompt is displayed immediately
}
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
  atexit(tty_restore_mode);  // Always restore terminal even on forced exit

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
  /* Run .shellrc if it exists
  FILE *fd = fopen(".shellrc", "r");
  if (fd) {
    YY_BUFFER_STATE shellrc_buf = yy_create_buffer(fd, YY_BUF_SIZE);
    yypush_buffer_state(shellrc_buf);

    Shell::_srcCmd = true;
    yyparse();
    //yy_delete_buffer(shellrc_buf);

    yypop_buffer_state();
    fclose(fd);

    Shell::_srcCmd = false;

    yyin = stdin;
  }*/

  if (isatty(0)) {
    Shell::prompt();
  }
  //Shell::prompt();
  yyparse(); // Start the parser which will read and execute commands
  tty_restore_mode();
  return 0;
}

// The parser and lexer use this to construct commands as they parse user input
Command Shell::_currentCommand;
bool Shell::_srcCmd;
