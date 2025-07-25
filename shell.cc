#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "shell.hh"
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
#endif
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


void Shell::prompt() {
  // Only print prompt if input is from a terminal and not sourcing a file (Part 1B.4: isatty())

  if ( isatty(0) & !_srcCmd ){
    // Support for variable prompt (Part 3.7: Variable prompt - extra credit 0.5pts)
    // If PROMPT environment variable is set, use it; otherwise use default
    char * pPrompt = getenv("PROMPT");
    if (pPrompt != NULL) printf("%s ", pPrompt);
  	else printf("myshell>"); // Default prompt
    fflush(stdout); // Ensure prompt is displayed immediately
  }
}

extern "C" void sigIntHandler(int sig)
{
  // Handle Ctrl-C signal (Part 2.1: Ctrl-C)
  // When Ctrl-C is typed and no command is running, discard current prompt and show fresh one
  if (sig == SIGINT) {
  	Shell::_currentCommand.clear(); 
  	printf("\n");
  	Shell::prompt();
  }

  // Handle child process termination signal (Part 2.2: Zombie Elimination)
  // This prevents zombie processes by cleaning up background processes when they finish
  if (sig == SIGCHLD) {
    pid_t pid;
	  int status;

    // Use WNOHANG to avoid blocking - check all finished children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
      // Search through our list of background processes
      for (unsigned i = 0; i < Shell::_bgPIDs.size(); i++) {
        if (pid == Shell::_bgPIDs[i]) {
            // Found the finished process - notify user and remove from list
            printf("[%d] exited\n", pid);
            Shell::_bgPIDs.erase(Shell::_bgPIDs.begin() + i);
            break;
        }
      }
    }
  }
}

// Default source file implementation (Part 2.7: Creating a Default Source File - extra credit 1pt)
// When shell starts, automatically run .shellrc if it exists
void source(void) {
  std::string s = ".shellrc"; // Look for .shellrc in current directory
  FILE * in = fopen(s.c_str(), "r");

  if (!in) {
    // File doesn't exist - just return silently
    return;
  }

  // Switch input buffer to read from .shellrc file (Part 2.6: source command)

  yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE));
  Shell::_srcCmd = true; // Flag to suppress prompts while sourcing
  yyparse(); // Parse and execute commands from file
  yypop_buffer_state(); // Restore original input buffer
  fclose(in); 
  Shell::_srcCmd = false; 
}

int main(int argc, char **argv) {
  // Set up signal handlers for proper shell behavior
  struct sigaction sa;
  sa.sa_handler = sigIntHandler; // Use my custom signal handler
  sigemptyset(&sa.sa_mask); // Don't block other signals during handler execution
  sa.sa_flags = SA_RESTART; // Automatically restart interrupted system calls
  
  // Run default source file on startup (Part 2.7: Default Source File - extra credit)
  source();
  
  // Set up special environment variables (Part 3.1: Environment variable expansion)
  // NOTE FOR SELF: ${$} - The PID of the shell process
  std::string s = std::to_string(getpid());
  setenv("$", s.c_str(), 1);
  
  // ${SHELL} - The path of the shell executable
  char abs_path[256];
  realpath(argv[0], abs_path); // Convert relative path to absolute path
  setenv("SHELL", abs_path, 1);

  // Initialize shell state
  Shell::_srcCmd = false; // Not currently sourcing a file
  Shell::prompt(); 
  
  // Install signal handlers (Part 2.1: Ctrl-C and Part 2.2: Zombie Elimination)
  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }
  if (sigaction(SIGCHLD, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }
  
  // Start the main parser loop
  yyrestart(stdin); // Set input source to stdin
  yyparse(); // Begin parsing and executing commands
}

/* Global shell state variables */
Command Shell::_currentCommand;    // Current command being parsed/executed
std::vector<int> Shell::_bgPIDs;   // List of background process PIDs for zombie cleanup (Part 2.2)
bool Shell::_srcCmd;           // Flag indicating if currently sourcing a file (Part 2.6/2.7)
bool Shell::_isSubshell = false;
