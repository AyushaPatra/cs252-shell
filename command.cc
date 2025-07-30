/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include "command.hh"
#include "shell.hh"

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>  // for PATH_MAX
#include <signal.h>
#include <string.h>

#include <pwd.h>
#include <string>




Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
    _clear = false;

    _lastPID = -1;
    _lastRT = -1;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if (_outFile == _errFile) {
	    if ( _outFile ) {
        	delete _outFile;
   	    }
	    _outFile = NULL;
	    _errFile = NULL;
    }
    else {

    	if ( _outFile ) {
    	    delete _outFile;
    	}
    	_outFile = NULL;

    	if ( _errFile ) {
        	delete _errFile;
    	}
    	_errFile = NULL;

    }
    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    _background = false;

    _append = false;

    _clear = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

extern char **environ;
std::string _lastARG;

std::string Command::getLastArg() {
	return _lastARG;
}

void handler(int sig)
{
	if (sig == SIGCHLD) {
		while(waitpid(-1, NULL, WNOHANG) > 0);
        }
}

void Command::execute() {
  // Part 1A: Main command execution function

  // Save current working directory for potential restoration later
  char origDir[PATH_MAX];
  getcwd(origDir, PATH_MAX);

  _clear = true; // Flag to indicate we should show prompt after execution
  
  // Don't do anything if there are no simple commands
  if ( _simpleCommands.size() == 0 ) {
    if (isatty(0)) {
      Shell::prompt(); // Show prompt again if running interactively
    }
    return;
  }

  // Part 1B: Set up I/O redirection infrastructure
  // Save original stdin, stdout, stderr for restoration after command execution

  int tmpin = dup(0);   // Save original stdin
  int tmpout = dup(1);  // Save original stdout  
  int tmperr = dup(2);  // Save original stderr
  
  int fdin; // File descriptor for input redirection

  // Part 1B.2: Handle input redirection (< file)
  if (_inFile) {
    fdin = open(_inFile->c_str(), O_RDONLY, 0777); // Open input file for reading
  }
  else {
    fdin = dup(tmpin); // Use original stdin if no input redirection
  }

  int ret; // Process ID for forked processes
  int fdout; // File descriptor for output redirection
  
  // Part 1B.2: Handle error redirection (2> file or 2>> file)
  if (_errFile) {
    int fderr;
    if (_append) {
      // 2>> file - append stderr to file
      fderr = open(_errFile->c_str(), O_APPEND | O_WRONLY | O_CREAT , 0777);
    }
    else {
      // 2> file - redirect stderr to file (truncate)
      fderr = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC , 0777);
    }
    dup2(fderr, 2); // Redirect stderr to the error file
    close(fderr);
  }
  
  // Part 1B.3: Main command loop - process each simple command in the pipeline
  for (unsigned int i = 0; i < _simpleCommands.size(); i++) {
    SimpleCommand * simpleCommand = _simpleCommands[i];

    // Part 2.5: Track last argument for $_ environment variable
    for (unsigned int i = 0; i < simpleCommand->_arguments.size(); i++) {
      _lastARG = simpleCommand->_arguments[i][0];	
    }

    // Part 2.1: Built-in command: setenv (set environment variables)
    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "setenv")) {
      if (simpleCommand->_arguments.size() < 3) {
        fprintf(stderr, "Usage: setenv VAR VALUE\n");
        continue;
      }
      setenv(simpleCommand->_arguments[1]->c_str(), simpleCommand->_arguments[2]->c_str(), 1);
      continue; // Built-ins don't need process creation
    }
    
    // Part 2.1: Built-in command: unsetenv (remove environment variables)
    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "unsetenv")) {
      if (simpleCommand->_arguments.size() < 2) {
        fprintf(stderr, "Usage: unsetenv VAR\n");
        continue;
      }
      unsetenv(simpleCommand->_arguments[1]->c_str());
      continue; // Built-ins don't need process creation
    }

    // Part 2.2: Built-in command: cd (change directory)
    // Extra credit Tilde expansion

    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "cd")) {
  const char* path = nullptr;
  char* freedPath = nullptr;

  if (simpleCommand->_arguments.size() == 1) {
    // Just 'cd' → go to $HOME
    path = getenv("HOME");
    if (!path) {
      fprintf(stderr, "cd: HOME not set\n");
      continue;
    }
  } else {
    const char* arg = simpleCommand->_arguments[1]->c_str();

    if (arg[0] == '~') {
      std::string expanded;

      if (arg[1] == '\0' || arg[1] == '/') {
        // ~ or ~/subdir
        const char* home = getenv("HOME");
        if (!home) {
          fprintf(stderr, "cd: HOME not set\n");
          continue;
        }
        expanded = std::string(home);
        if (arg[1] == '/')
          expanded += (arg + 1);
      } else {
        // ~username or ~username/...
        std::string user;
        int i = 1;
        while (arg[i] && arg[i] != '/') {
          user += arg[i];
          i++;
        }

        struct passwd* pw = getpwnam(user.c_str());
        if (!pw) {
          fprintf(stderr, "cd: no such user %s\n", user.c_str());
          continue;
        }

        expanded = pw->pw_dir;
        if (arg[i] == '/')
          expanded += (arg + i);
      }

      freedPath = strdup(expanded.c_str());


      path = freedPath;
    } else {
      // Regular path
      path = arg;
    }
  }

  if (chdir(path) != 0) {
    perror("cd: can't cd to notfound");
  }

  if (freedPath) {
    free(freedPath);
  }

  continue;
}


    // Part 1B.3: Set up input for this command from previous command in pipeline
    dup2(fdin, 0); // Redirect stdin to input from previous command or file
    close(fdin);

    // Part 1B.2: Set up output redirection for this command
    if (i == _simpleCommands.size()-1) {
      // Last command in pipeline - handle final output redirection
      if (_outFile) {
        if (_append) {
          // >> file - append to output file
          fdout = open(_outFile->c_str(), O_APPEND | O_WRONLY | O_CREAT , 0777);
        }
        else {
          // > file - redirect to output file (truncate)
          fdout = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC  , 0777);
        }
      }
      else {
        fdout = dup(tmpout); // Use original stdout if no output redirection
      }
    }
    else {
      // Not the last command - create pipe to next command (Part 1B.3: Pipes)
      int fdpipe[2];
      pipe(fdpipe);
      fdout = fdpipe[1]; // Write end of pipe
      fdin = fdpipe[0];  // Read end will be input for next command
    }
    
    // Redirect stdout to the output destination (file or pipe)
    dup2(fdout, 1);
    close(fdout);
    
    // Part 1A: Fork and execute the command
    ret = fork();
    if (ret == 0) {
      // Child process - execute the command
      
      // Part 2.3: Built-in command: printenv (print all environment variables)
      if (!strcmp(simpleCommand->_arguments[0]->c_str(), "printenv")) {
        char **p=environ;
        while (*p!=NULL) {
          printf("%s\n",*p);
          p++;
        }
        exit(0); // Built-in commands exit directly from child
      }

      // Part 1A: Prepare arguments array for execvp
      char ** ch = new char*[simpleCommand->_arguments.size()+1];
      
      for (unsigned int i = 0; i < simpleCommand->_arguments.size(); i++) {
        ch[i] = (char*)simpleCommand->_arguments[i]->c_str();
      }
      ch[simpleCommand->_arguments.size()] = NULL; // Null-terminate the array

      // Part 1A: Execute the command using execvp (searches PATH)
      execvp(simpleCommand->_arguments[0]->c_str(), ch);

      // If execvp fails, handle on_error
char *on_error = getenv("on_error");
if (on_error) {
  printf("Command failed. Running on_error: %s\n", on_error);
  execlp("/bin/sh", "sh", "-c", on_error, (char *)NULL);
}

      perror("execvp"); // Only reached if execvp fails
      exit(1);
    }
    
    // Part 1A: Parent process - set up signal handling for background processes
    signal(SIGCHLD, handler);
  }

  // Part 1B: Restore original stdin, stdout, stderr after all commands complete
  
  dup2(tmpin, 0);   // Restore original stdin
  dup2(tmpout, 1);  // Restore original stdout
  dup2(tmperr, 2);  // Restore original stderr
  close(tmpin);
  close(tmpout);
  close(tmperr);
  close(fdin);

  // Part 1A: Handle foreground vs background processes
  if (!_background) {
    // Foreground process - wait for completion and get exit status
    int temp;
    waitpid(ret, &temp, 0);

    // Check if last simple command exited with non-zero
if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
  const char* err = getenv("ON_ERROR");
  if (err) {
    printf("%s\n", err);
  }
}
    _lastRT = WEXITSTATUS(temp); // Part 2.4: Store exit status for $? variable
  }
  else {
    // Background process - store PID for $! variable (Part 2.4)
    _lastPID = ret;
  }

  // Prepare for next command iteration
  if (_clear) {
    if (isatty(0)) {
      Shell::prompt(); // Show prompt for interactive sessions
    }
  }

  clear(); // Clean up command data structures for next command
}

SimpleCommand * Command::_currentSimpleCommand;
