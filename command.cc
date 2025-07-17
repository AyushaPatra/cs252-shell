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
#include <cstring>

#include <unistd.h>     // for fork(), execvp()
#include <sys/wait.h>   // for waitpid()
#include <fcntl.h>      // for open(), O_CREAT, O_WRONLY, etc.

#include <iostream>

#include "command.hh"
#include "shell.hh"


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
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

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
    _append = false;
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

void Command::execute() {

    // Don't do anything if there are no simple commands
    if (_simpleCommands.size() == 0) {
        Shell::prompt();
        return;
    }

    // Handle built-in "exit" command
    if (_simpleCommands.size() == 1 &&
        _simpleCommands[0]->_arguments.size() == 1 &&
        *_simpleCommands[0]->_arguments[0] == "exit") {
        printf("Biee!\n");
        exit(0);
    }

    // Save default FDs: stdin, stdout, stderr so we can restore later
    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    // Set up input redirection or default stdin: : use input file if provided
    int fdin = _inFile ? open(_inFile->c_str(), O_RDONLY) : dup(defaultin);

    // Set up error redirection or default stderr: to error file if provided
    int fderr = _errFile ? open(_errFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666) : dup(defaulterr);
    dup2(fderr, 2);
    if (fderr != 2) close(fderr); // prevent fd leak

    std::vector<pid_t> pids; // To track all child process IDs
    int numCmds = _simpleCommands.size();
    int fdpipe[2]; // Pipe array: used between commands

    for (int i = 0; i < numCmds; ++i) {
        // stdin is taken from current fdin (may be file or pipe)
        dup2(fdin, 0);
        if (fdin != 0) close(fdin); // Close after duping to prevent leak

        // Setup output for the last command or pipe
        int fdout;
        if (i == numCmds - 1) {
            // Last command: write to file or stdout
            if (_outFile) {
                fdout = open(_outFile->c_str(),
                             _append ? O_WRONLY | O_CREAT | O_APPEND
                                     : O_WRONLY | O_CREAT | O_TRUNC,
                             0666);
            } else {
                fdout = dup(defaultout);
            }
        } else {
            // Not last command: setup pipe to next
            pipe(fdpipe);
            fdout = fdpipe[1];  // This command writes to pipe
            fdin = fdpipe[0];   // Next command reads from pipe
        }

        dup2(fdout, 1);  // stdout to output file/pipe
        if (fdout != 1) close(fdout);

        // Fork child  process to run the command
        pid_t pid = fork();
        if (pid == 0) {
            // In child process: build argv and exec
            // Convert arguments to char*[]
            SimpleCommand *sc = _simpleCommands[i];
            int argc = sc->_arguments.size();
            char **argv = new char *[argc + 1];
            for (int j = 0; j < argc; ++j) {
                argv[j] = (char *)sc->_arguments[j]->c_str();
            }
            argv[argc] = nullptr;

            //execvp(argv[0], argv);
            if (strchr(argv[0], '/')) {
                execv(argv[0], argv);  // execv: full path command like ./script.sh
            } else {
                execvp(argv[0], argv); // execvp: normal shell commands like ls, grep
            }
            //  If exec fails
            perror("execvp failed");
            exit(1);
        } else if (pid < 0) {
            perror("fork failed");
            exit(2);
        } else {
            //  Parent process: track the child's PID
            pids.push_back(pid);
        }

        // Parent: close pipe write end if it's not the last command
        if (i != numCmds - 1) {
            close(fdpipe[1]);
        }
    }

    // Restore defaults: stdin, stdout, stderr
    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    // Wait for children unless background (&)
    if (!_background) {
        for (pid_t pid : pids) {
            waitpid(pid, nullptr, 0);
        }
    }

    // Reset the command data structure
    clear();

    // Prompt again for the next command
    Shell::prompt();
}


void Command::prompt() {
    if (isatty(0)) {
        printf("myshell>");
        fflush(stdout);
    }
}

SimpleCommand * Command::_currentSimpleCommand;
