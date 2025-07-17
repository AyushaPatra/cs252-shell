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
    std::vector<int> pids;

    if (_simpleCommands.size() == 0) {
        Shell::prompt();
        return;
    }

    // Save original file descriptors
    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    // Setup input file if specified
    int fdin = _inFile ? open(_inFile->c_str(), O_RDONLY) : dup(defaultin);

    // Setup error redirection
    int fderr = _errFile ? open(_errFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666) : dup(defaulterr);
    dup2(fderr, 2);
    close(fderr);  // Close after dup2

    int numCommands = _simpleCommands.size();
    int fdpipe[2];

    for (int i = 0; i < numCommands; ++i) {
        // Set up input
        dup2(fdin, 0);
        close(fdin); // Close original fdin

        // Set up output
        int fdout;
        if (i == numCommands - 1) {
            // Last command
            fdout = _outFile ? open(_outFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666) : dup(defaultout);
        } else {
            // Middle command: create pipe
            if (pipe(fdpipe) < 0) {
                perror("pipe failed");
                exit(2);
            }
            fdout = fdpipe[1];     // Write end
            fdin = fdpipe[0];      // Save read end for next iteration
        }

        dup2(fdout, 1);
        close(fdout); // Done with original write end

        // Fork
        int pid = fork();
        if (pid == 0) {
            // In child: build argv
            SimpleCommand *sc = _simpleCommands[i];
            int argc = sc->_arguments.size();
            char **argv = new char *[argc + 1];
            for (int j = 0; j < argc; ++j) {
                argv[j] = (char *)sc->_arguments[j]->c_str();
            }
            argv[argc] = nullptr;

            execvp(argv[0], argv);
            perror("execvp failed");
            exit(1);
        } else if (pid < 0) {
            perror("fork failed");
            exit(2);
        } else {
            pids.push_back(pid);
        }
    }

    // Restore file descriptors
    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    // Wait for children unless background
    if (!_background) {
        for (int pid : pids) {
            waitpid(pid, nullptr, 0);
        }
    }

    clear();
    Shell::prompt();
}


void Command::prompt() {
    if (isatty(0)) {
        printf("myshell>");
        fflush(stdout);
    }
}

SimpleCommand * Command::_currentSimpleCommand;
