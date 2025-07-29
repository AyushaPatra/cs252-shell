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
	_clear = true;
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        if (isatty(0)) {
	    Shell::prompt();
	}
        return;
    }

    // Add execution here
    // For every simple command fork a new process


    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);
    
    int fdin;


    if (_inFile) {
	    fdin = open(_inFile->c_str(), O_RDONLY, 0777);
    }
    else {
	    fdin = dup(tmpin);
    }

    int ret;
    int fdout;
    if (_errFile) {
	    int fderr;
	    if (_append) {
		    fderr = open(_errFile->c_str(), O_APPEND | O_WRONLY | O_CREAT , 0777);
	    }
	    else {
            	    fderr = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC , 0777);
	    }
	    dup2(fderr, 2);
	    close(fderr);
    }
    for (unsigned int i = 0; i < _simpleCommands.size(); i++) {
	    SimpleCommand * simpleCommand = _simpleCommands[i];

	    for (unsigned int i = 0; i < simpleCommand->_arguments.size(); i++) {
            		_lastARG = simpleCommand->_arguments[i][0];	
	    }


            if (!strcmp(simpleCommand->_arguments[0]->c_str(), "setenv")) {
                        setenv(simpleCommand->_arguments[1]->c_str(), simpleCommand->_arguments[2]->c_str(), 1);
                        continue;
            }
            if (!strcmp(simpleCommand->_arguments[0]->c_str(), "unsetenv")) {
                    unsetenv(simpleCommand->_arguments[1]->c_str());
                    continue;
            }
            if (!strcmp(simpleCommand->_arguments[0]->c_str(), "cd")) {
                    if (simpleCommand->_arguments[1] == NULL) {
                            chdir("/homes/ding190");
                            continue;
		    }
		    else if (strcmp(simpleCommand->_arguments[1]->c_str(), "${HOME}") == 0) {
			    chdir("/homes/ding190");
                            continue;
		    }
		    
		    const char * loc = simpleCommand->_arguments[1]->c_str();
		    char msg[100] = "cd: can't cd to ";
		    strcat(msg, loc);
	    	    int error = chdir(loc);	    
		    if (error != 0) {
			    perror(msg);
		    }
		    continue;
	    }

	    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "source")) {
                        FILE * fd = fopen(simpleCommand->_arguments[1]->c_str(), "r");
                        if (fd == 0) {
                                continue;
                        }

                        char buf[1000];
                        //fgets(buf, 2000, fd);
                        //fgets(buf, 2000, fd);
                        void * ret;
                        while (true) {

                        memset(buf, 0, sizeof(buf));
                        ret = fgets(buf, 1000, fd);
                        if (ret == NULL) {
                                break;
                        }
                        buf[strlen(buf)-1] = '\0';
			
                        char set[7] = "setenv";
                        if (strstr(buf, set) != NULL) {
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
                                setenv(name, val, 1);
                                free(name);
                                free(val);
                                continue;
                        }

                        int tmpin = dup(0);
                        int tmpout = dup(1);

			int fdpipein[2];
                        int fdpipeout[2];

                        pipe(fdpipein);
                        pipe(fdpipeout);

                        //printf("buf is %s\n", buf);

                        write(fdpipein[1], buf, strlen(buf));
                        write(fdpipein[1], "\n", 1);
                        close(fdpipein[1]);

                        dup2(fdpipein[0], 0);
                        close(fdpipein[0]);
                        dup2(fdpipeout[1], 1);
                        close(fdpipeout[1]);

                        int ret = fork();
                        if (ret == 0) {
                                execvp("/proc/self/exe", NULL);
                                perror("execvp");
                                exit(1);
                        }

                        dup2(tmpin, 0);
                        dup2(tmpout, 1);
                        close(tmpin);
                        close(tmpout);

                        char ch;
                        char * buf1 = new char[5000];
                        char * buf2 = buf1;
                        //char buf3[5] = "hola";
                        //printf("before enter while\n");
                        while (read(fdpipeout[0], &ch, 1)) {
                                //printf("ch is %s\n", ch);
                                if ( ch != '\n') {
                                        //printf("ch is %s\n", ch);
                                         *buf2 = ch;
                                         buf2++;
                                         //printf("buf1 is %s\n", buf1);
                                }
                        }
                        *buf2 = '\0';
                        printf("%s\n", buf1);
                        //printf("%s\n", buf3);
                        }
                        fclose(fd);
			continue;
                }

	    dup2(fdin, 0);
	    close(fdin);

	    if (i == _simpleCommands.size()-1) {
		    if (_outFile) {
			    if (_append) {
				    fdout = open(_outFile->c_str(), O_APPEND | O_WRONLY | O_CREAT , 0777);
			    }
			    else {
			    	    fdout = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC  , 0777);
		    
			    }
		    }
		    else {
			    fdout = dup(tmpout);
		    }
	    }
	    else {
			int fdpipe[2];
                        pipe(fdpipe);
                        fdout = fdpipe[1];
                        fdin = fdpipe[0];
	    }
	    
	    dup2(fdout, 1);
	    close(fdout);
	    
	    
	    ret = fork();
	    if (ret == 0) {
		    if (!strcmp(simpleCommand->_arguments[0]->c_str(), "printenv")) {
			char **p=environ;
			while (*p!=NULL) {
				printf("%s\n",*p);
				p++;
			}
			exit(0);
		    }


		
		    char ** ch = new char*[simpleCommand->_arguments.size()+1];
		   
		    for (unsigned int i = 0; i < simpleCommand->_arguments.size(); i++) {
			    ch[i] = (char*)simpleCommand->_arguments[i]->c_str();
		    }
		    ch[simpleCommand->_arguments.size()] = NULL;

		    execvp(simpleCommand->_arguments[0]->c_str(), ch);
		    perror("execvp");
		    exit(1);
	    }
	    signal(SIGCHLD, handler);
    }

    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);
    close(fdin);

    if (!_background) {
	    int temp;
	    waitpid(ret, &temp,0 );
	    _lastRT = WEXITSTATUS(temp);
    }
    else {
	    _lastPID = ret;
    }
    // Setup i/o redirection
    // and call exec

    // Clear to prepare for next command
    if (_clear) {
	    if (isatty(0)) {
        	Shell::prompt();
    	    }
    }

    clear();

    // Print new prompt
    /*
    if (isatty(0)) {
    	Shell::prompt();
    }
    */
    
}

SimpleCommand * Command::_currentSimpleCommand;
