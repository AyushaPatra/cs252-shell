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
      setenv(simpleCommand->_arguments[1]->c_str(), simpleCommand->_arguments[2]->c_str(), 1);
      continue; // Built-ins don't need process creation
    }
    
    // Part 2.1: Built-in command: unsetenv (remove environment variables)
    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "unsetenv")) {
      unsetenv(simpleCommand->_arguments[1]->c_str());
      continue; // Built-ins don't need process creation
    }
    
    // Part 2.2: Built-in command: cd (change directory)
    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "cd")) {
      if (simpleCommand->_arguments[1] == NULL) {
        // cd with no arguments - go to home directory
        chdir("/homes/patra10");
        continue;
      }
      else if (strcmp(simpleCommand->_arguments[1]->c_str(), "${HOME}") == 0) {
        // Handle ${HOME} expansion in cd command
        chdir("/homes/ding190");
        continue;
      }
      
      // cd to specified directory with error handling
      const char * loc = simpleCommand->_arguments[1]->c_str();
      char msg[100] = "cd: can't cd to ";
      strcat(msg, loc);
      int error = chdir(loc);	    
      if (error != 0) {
        perror(msg); // Print error if directory change fails
      }
      continue;
    }

    /* Part 2.6: Built-in command: source (execute commands from file)
    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "source")) {
      Shell::_srcCmd = true;

      FILE * fd = fopen(simpleCommand->_arguments[1]->c_str(), "r");
      if (fd == 0) {
        Shell::_srcCmd = false;
        continue; // Skip if file cannot be opened
      }

      char buf[1000];
      void * ret;
      
      // Read and execute each line from the source file
      while (true) {
        memset(buf, 0, sizeof(buf));
        ret = fgets(buf, 1000, fd);
        if (ret == NULL) {
          break; // End of file reached
        }
        buf[strlen(buf)-1] = '\0'; // Remove trailing newline
        
        // Special handling for setenv commands in source files
        char set[7] = "setenv";
        if (strstr(buf, set) != NULL) {
          // Parse setenv command manually from source file
          char * name = (char*)malloc(50);
          char * n = name;
          char * val = (char*)malloc(50);
          char * v = val;
          char * t = strchr(buf, ' ');
          t++;
          while (*t != ' ') {
            *n = *t;
            n++;
            t++;
          }
          *n = '\0';
          t++;
          while (*t != '\0') {
            *v = *t;
            v++;
            t++;
          }
          *v = '\0';
          setenv(name, val, 1); // Set the environment variable
          free(name);
          free(val);
          continue;
        }

        // For other commands, execute them through subshell
        int tmpin = dup(0);
        int tmpout = dup(1);

        int fdpipein[2];
        int fdpipeout[2];

        pipe(fdpipein);
        pipe(fdpipeout);

        // Send command to subshell through pipe
        write(fdpipein[1], buf, strlen(buf));
        write(fdpipein[1], "\n", 1);
        close(fdpipein[1]);

        // Set up pipes for subshell communication
        dup2(fdpipein[0], 0);
        close(fdpipein[0]);
        dup2(fdpipeout[1], 1);
        close(fdpipeout[1]);

        // Create subshell to execute the command
        int ret = fork();
        if (ret == 0) {
          execvp("/proc/self/exe", NULL); // Execute same shell binary
          perror("execvp");
          exit(1);
        }

        // Restore original stdin/stdout
        dup2(tmpin, 0);
        dup2(tmpout, 1);
        close(tmpin);
        close(tmpout);

        // Read and display output from subshell
        char ch;
        char * buf1 = new char[5000];
        char * buf2 = buf1;
        while (read(fdpipeout[0], &ch, 1)) {
          //if ( ch != '\n') {
            *buf2 = ch;
            buf2++;
          //}
        }
        *buf2 = '\0';
        printf("%s", buf1); // Print subshell output
      }

      fclose(fd);
      Shell::_srcCmd = false;
      continue; // Source command is complete
    }*/

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
