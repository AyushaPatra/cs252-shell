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

void Command::redirect(int i, std::string * curr) {
    if (i==0) {
        if ( _inFile ) {
            printf("Ambiguous input redirect.\n");
            exit(1);
        } else {
            _inFile = curr;
        }
    }
    if (i==2) {
        if ( _errFile ) {
            printf("Ambiguous error redirect.\n");
            exit(1);
        } else {
            _errFile = curr;
        }
    }
    if (i==1) {
        if ( _outFile ) {
            printf("Ambiguous output redirect.\n");
            exit(1);
        } else {
            _outFile = curr;
        }
    }
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if (_simpleCommands.size() == 0) {
        Shell::prompt();
        return;
    }

    // Print contents of Command data structure
    //print();

    // exit myshell
    std::string* cmd = _simpleCommands[0]->_arguments[0];
    if ( !strcmp(cmd->c_str(),"exit") ) {
            printf( "Good bye :)\n");
        exit(1);
    }

    // Save default stdin, stdout, stderr
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);

    // Set up input redirection or use default
    int fdin;
    if (_inFile) {
        const char* infile = _inFile->c_str();
        fdin = open(infile, O_RDONLY);
        if (fdin < 0) {
            fprintf(stderr, "/bin/sh: 1: cannot open %s: No such file\n", infile);
            Shell::prompt();
            clear();
            return;
        }
    } else {
        fdin = dup(tmpin);
    }

    int fdout;
    int fderr;
    int ret = -1;

    // Loop through each simple command
    for (size_t i = 0; i < _simpleCommands.size(); ++i) {
        // Redirect input
        dup2(fdin, 0);
        close(fdin);

        // Setup output
        if (i == _simpleCommands.size() - 1) {
            // Last command: handle output redirection
            if (_outFile) {
                const char* outfile = _outFile->c_str();
                if (_append) {
                    fdout = open(outfile, O_CREAT | O_WRONLY | O_APPEND, 0664);
                } else {
                    fdout = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                }
            } else {
                fdout = dup(tmpout);
            }

            // Handle error redirection
            if (_errFile) {
                const char* errfile = _errFile->c_str();
                if (_append) {
                    fderr = open(errfile, O_CREAT | O_WRONLY | O_APPEND, 0664);
                } else {
                    fderr = open(errfile, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                }
            } else {
                fderr = dup(tmperr);
            }

            dup2(fderr, 2);
            close(fderr);

            // Set $_ to last argument
            int n = _simpleCommands[i]->_arguments.size();
            char* c = strdup(_simpleCommands[i]->_arguments[n - 1]->c_str());
            setenv("_", c, 1);
        } else {
            // Intermediate command: set up pipe
            int fdpipe[2];
            pipe(fdpipe);
            fdout = fdpipe[1];
            fdin = fdpipe[0]; // used in next iteration
        }

        // Redirect output
        dup2(fdout, 1);
        close(fdout);

        // Fork child
        ret = fork();
        if (ret == 0) {
            // Build argv array
            const char* command = _simpleCommands[i]->_arguments[0]->c_str();
            int argnum = _simpleCommands[i]->_arguments.size();
            char** args = new char*[argnum + 1];
            for (int j = 0; j < argnum; ++j) {
                args[j] = (char*)_simpleCommands[i]->_arguments[j]->c_str();
            }
            args[argnum] = NULL;

            execvp(command, args);
            perror("execvp");
            exit(1);
        } else if (ret < 0) {
            perror("fork");
            return;
        }
    }

    // Restore stdin, stdout, stderr
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);

    // Wait for the last child process to finish
    if (!_background) {
        int status;
        waitpid(ret, &status, 0);
    } else {
        // allow background PID tracking
        printf("Backgrounded process: %d\n", ret);
    }

    // Clear command
    clear();

    // Show next prompt
    //Shell::prompt();
}


SimpleCommand * Command::_currentSimpleCommand;
