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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h> 
#include <errno.h>
#include <string.h>
#include <string>
#include <stdlib.h>


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
    // This handles the case where user just presses enter with no command
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    // Print contents of Command data structure
    //print();

    // exit myshell
    std::string* cmd = _simpleCommands[0]->_arguments[0];
    
    // Built-in command: exit - terminates the shell (Part 2.3: Exit)
    if ( !strcmp(cmd->c_str(),"exit") ) {
        if (!Shell::_isSubshell) {
           printf("Bieee :)\n");
        }
        exit(1); // Exit with status 1
    }

    // Built-in command: setenv - sets environment variables (Part 2.6: Builtin Functions)
    // Format: setenv VARIABLE_NAME VALUE
    if ( !strcmp(cmd->c_str(),"setenv") ) {
        // Validate argument count - setenv needs exactly 2 arguments plus command name
        if ( _simpleCommands[0]->_arguments.size() != 3 ) {
            fprintf(stderr, "setenv should take two arguments\n");
            return;
        }
        // Extract variable name and value from arguments
        const char * A = _simpleCommands[0]->_arguments[1]->c_str(); // Variable name
        const char * B = _simpleCommands[0]->_arguments[2]->c_str(); // Variable value
        setenv(A, B, 1); // Set environment variable (1 means overwrite if exists)
        clear(); // Clean up command structure
        Shell::prompt(); // Show next prompt
        return;
    }

    // Built-in command: unsetenv - removes environment variables (Part 2.6: Builtin Functions)
    if ( !strcmp(cmd->c_str(),"unsetenv") ) {
        const char * A = _simpleCommands[0]->_arguments[1]->c_str(); // Variable to unset
        unsetenv(A); // Remove the environment variable
        clear();
        Shell::prompt();
        return;
    }

    // Built-in command: cd - changes current directory (Part 2.6: Builtin Functions)
    if ( !strcmp(cmd->c_str(),"cd") ) {
        if (_simpleCommands[0]->_arguments.size()==1) {
            // No directory specified - go to home directory
            chdir(getenv("HOME"));
        } else {
            // Directory specified - change to that directory
            const char * path = _simpleCommands[0]->_arguments[1]->c_str();
            chdir(path);
        }
        clear();
        Shell::prompt();
        return;
    }


    // Atp we have external commands to execute (Part 1B: Executing commands)
    // Save the original stdin/stdout/stderr so we can restore them later

    int tmpin = dup(0);   // Save original stdin
    int tmpout = dup(1);  // Save original stdout
    int tmperr = dup(2);  // Save original stderr
    int numsimplecommands = _simpleCommands.size(); // Number of commands in pipeline
    
    // Set up initial input for the pipeline (Part 1B.2: File redirection)
    // If input redirection is specified, open that file; otherwise use stdin
    int fdin;
    if ( _inFile ) {
        // Input redirection specified (< filename)
        const char* infile = _inFile->c_str();
        fdin = open(infile, O_RDONLY); // Open file for reading only
        if (fdin < 0) {
            // File doesn't exist or can't be opened
            fprintf(stderr, "/bin/sh: 1: cannot open %s: No such file\n", infile);
            Shell::prompt();
            clear();
            return;
        }
    } 
    else {
        // No input redirection - use default input (stdin)
        fdin = dup(tmpin);
    }
    // Main execution loop - process each command in the pipeline (Part 1B.1 & 1B.3: Process creation and Pipes)
    // Each iteration handles one simple command and sets up pipes for the next

    int ret; // Will store the PID of the last process created
    int fdout; // Output file descriptor for current command
    int fderr; // Error file descriptor for current command
    
    for (int i=0; i<numsimplecommands; i++) {
        
        dup2(fdin, 0); // Redirect stdin to fdin
        close(fdin);  
        
        // Set up output redirection for current command (Part 1B.2: File redirection)
        if (i == numsimplecommands-1) {
            // This is the LAST command in the pipeline
            // Handle output redirection to files if specified
            if ( _outFile ) {
                // Output redirection specified (> or >> filename)
                const char* outfile = _outFile->c_str();
                if ( _append ){
                    // Append mode (>>) - Part 1B.2 supports >> and >>& 
                    fdout = open(outfile, O_CREAT|O_WRONLY|O_APPEND, 0664);
                }
                else {
                    // Overwrite mode (>) - Part 1B.2 supports > and >&
                    fdout = open(outfile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
                }
            } 
            else {
                // No output redirection - use default output (stdout)
                fdout = dup(tmpout);
            }

            // Handle error redirection for last command (Part 1B.2: stderr redirection 2>, >&, >>& )
            if ( _errFile ) {
                // Error redirection specified (2> or 2>> filename)
                const char* errfile = _errFile->c_str();
                if ( _append ){
                    fderr = open(errfile, O_CREAT|O_WRONLY|O_APPEND, 0664);
                }
                else {
                    fderr = open(errfile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
                }
            }
            else {
                // No error redirection - use default stderr
                fderr = dup(tmperr);
            }
            dup2(fderr, 2); // Redirect stderr
            close(fderr);
            
            // Set the $_ environment variable to the last argument of last command (Part 3.1: Environment variable expansion)
           
            int n = _simpleCommands[i]->_arguments.size();
            char * c = strdup(_simpleCommands[i]->_arguments[n-1]->c_str());
            setenv("_", c, 1);
        }
        else {
            // This is NOT the last command - create a pipe to next command (Part 1B.3: Pipes)
            // The output of this command becomes input for the next command

            int fdpipe[2];
            pipe(fdpipe);    
            fdout = fdpipe[1]; 
            fdin = fdpipe[0]; 
        }
        
        
        dup2(fdout, 1); // Redirect stdout to fdout
        close(fdout);   // Close original descriptor

        // Create child process to execute the command (Part 1B.1: Simple command process creation and execution)

        ret = fork();
        if (ret==0) {
            // CHILD PROCESS 
            
            // Handle special built-in command: printenv (Part 2.6: Builtin Functions)

            if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv")) {
                // Print all environment variables
                char **p = environ; // environ is global array of environment strings
                while (*p!=NULL) {
                    printf("%s\n", *p);
                    p++;
                }
                exit(0); // Child exits after printing environment
            }
            
            // Prepare arguments for execvp() (Part 1B.1: Simple command process creation and execution)
            
            const char * command = _simpleCommands[i]->_arguments[0]->c_str();
            int argnum = _simpleCommands[i]->_arguments.size();
            char ** args = new char*[argnum+1]; // +1 for NULL terminator
            
            // Copy all arguments to the args array
            for (int j=0; j<argnum; j++) {
                args[j] = (char *)_simpleCommands[i]->_arguments[j]->c_str();
            }
            args[argnum] = NULL; // NULL-terminate the array
            
            // Execute the command - this replaces the child process image
            execvp(command, args);
            // If we reach here, execvp failed!
            perror("execvp");
            exit(1); // Exit child with error status
        } 
        else if (ret < 0) {
            // FORK FAILED 
            perror("fork");
            return;
        }
        // PARENT PROCESS continues here (ret > 0, ret = child PID)
        // The parent continues to set up the next command in the pipeline
    }
    
    // Restore original file descriptors

    dup2(tmpin,0);  // Restore original stdin
    dup2(tmpout,1); // Restore original stdout
    dup2(tmperr,2); // Restore original stderr
    close(tmpin);   // Close the saved descriptors
    close(tmpout);
    close(tmperr);

    //Handle process completion based on background flag
    if (!_background) {
        // FOREGROUND EXECUTION - wait for the last command to finish (Part 1B.1: Simple command process creation and execution)
        
        int status;
        waitpid(ret, &status, 0); // Wait for last child process
        
        // Set $? environment variable to exit status of last command (Part 3.1: Environment variable expansion)
        
        std::string s = std::to_string(WEXITSTATUS(status));
        setenv("?", s.c_str(), 1);
        
        // Handle ON_ERROR environment variable (Part 3.7: Variable prompt)
        
        char * pError = getenv("ON_ERROR");
        if (pError != NULL && WEXITSTATUS(status) != 0) printf("%s\n", pError);
    } else {
        // BACKGROUND EXECUTION - don't wait, just track the PID (Part 1A: Grammar supports & for background)
        // Set $! environment variable to PID of background process (Part 3.1: Environment variable expansion)
        std::string s = std::to_string(ret);
        setenv("!", s.c_str(), 1);
        // Add to list of background processes for zombie cleanup (Part 2.2: Zombie Elimination)
        Shell::_bgPIDs.push_back(ret);
    }

    // Clean up and prepare for next command
    clear(); // Free memory used by current command structure

    // Print new prompt for next command
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
