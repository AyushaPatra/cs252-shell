
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */
%x src

%{

#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.hh"
#include "shell.hh"
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

extern "C" char * read_line();

// Custom getc function for line editing support (Part 3.4: Edit mode)
// Integrates with read-line functionality for command line editing
int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    // Input is not from terminal - using standard getc (for file input/testing)
    return getc(f);
  }

  // Input is from terminal - using my line editor
  if (p==NULL || *p == 0) {
    char * s = read_line(); // Get line from my custom line editor
    p =s;
  }
  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

%}

%option noyywrap

%%

\n {
  // End of command line - return newline token for parser
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs - whitespace separation between tokens */
}

">" {
  // Simple output redirection (Part 1B.2: File redirection)
  return GREAT;
}

"<" {
  // Input redirection (Part 1B.2: File redirection)
  return LESS;
}

"|" {
  // Pipe operator for command chaining (Part 1B.3: Pipes)
  return PIPE;
}

"&" {
  // Background process indicator (Part 1A: Grammar supports background with &)
  return AMPERSAND;
}

">>" {
  // Append output redirection (Part 1B.2: File redirection supports >>)
  return GREATGREAT;
}

">&" {
  // Redirect both stdout and stderr (Part 1B.2: File redirection supports >&)
  return GREATAND;
}

"2>" {
  // Redirect stderr only (Part 1B.2: File redirection supports 2>)
  return TWOGREAT;
}

">>&" {
  // Append both stdout and stderr (Part 1B.2: File redirection supports >>&)
  return GREATGREATAND;
}

source {
  BEGIN(src); // Switch to source state to read filename
}

<src>[^ \t\n][^ \t\n]* {
  FILE * in = fopen(yytext, "r+");

  if (!in) {
    perror("fopen");
    BEGIN(INITIAL);
  } else {
    // Execute commands from the specified file
    fputc('\n', in); // Ensure file ends with newline
    yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE)); // Switch input buffer
    BEGIN(INITIAL); // Return to normal parsing state
    Shell::_srcCmd = true; 
    yyparse(); 
    yypop_buffer_state(); 
    fclose(in); 
    Shell::_srcCmd = false; 
  }
}

~[^ ]*\/[^ ]* {
// Tilde expansion with explicit path (Part 2.7: "~user/path" expansion)
// This handles cases like "~someuser/Documents" to expand to "/home/someuser/Documents"
  char * s = yytext+1; // Skip the '~' character to get username
  int i = 0;
  char * user = (char *) malloc(64); // Allocate space for username
  while (s[i] != '/' && s[i] != '\0') {
    user[i] = s[i]; // Copy username character by character
    i++;
  }
  user[i] = '\0'; // Null-terminate username
  
  // Look up user in password database to get home directory
  struct passwd * pwent = getpwnam(user);
  if (pwent) {
    // User found - construct full path with user's home directory
    char *result = (char *)malloc(strlen(pwent->pw_dir) + strlen(s+i) + 1);
    strcpy(result, pwent->pw_dir); // Copy home directory
    strcat(result, s+i);           // Append the path after username
    // Insert expanded path back into input stream in reverse order
    for (int j = strlen(result) - 1; j >= 0; --j) {
      myunputc(result[j]);
    }
    free(result);
  } else {
    // User not found - return original text unchanged
    for (int j = strlen(yytext) - 1; j >= 0; --j) {
      myunputc(yytext[j]);
    }
  }
  free(user);
}
  std::string str = std::string(yytext);
  std::string home = getenv("HOME");
  if (str.size() == 1) {
    // Just ~ - expand to current user's home directory
    yylval.cpp_string = new std::string(home);
  } 
  else {
    if (str[1] == '/') {
      // ~/path - expand to current user's home + path
      str = home + str.substr(1, -1);
    } 
    else {
      // ~user or ~user/path - expand to specified user's home directory
      str = std::string("/homes/") + str.substr(1,-1);
    }
    yylval.cpp_string = new std::string(str);
  }
  return WORD;


\"[^\n\"]*\" {
  std::string str = std::string(yytext);
  str = str.substr(1,str.size()-2); // Remove surrounding quotes
  yylval.cpp_string = new std::string(str);
  return WORD;
}

[^ \\\t\n]*(\\[^nt])((\\[^nt])|[^ \\\t\n])* {
  std::string str = std::string(yytext);
  std::size_t found = str.find('\\');
  // Remove all backslashes - the character after backslash is treated literally
  while(found!=-1) {
    str = str.substr(0,found) + str.substr(found+1,std::string::npos);
    found = str.find('\\', found+1);
  }
  yylval.cpp_string = new std::string(str);
  return WORD;
}

$\{[^ \}\n]*\}([^ \n]*|$\{[^ \}\n]*\})* {
  std::string str = std::string(yytext);
  std::string result = "";
  std::size_t found = str.find('$');
  // Process all ${...} expansions in the string
  while(found!=-1) {
    std::string component;
    result += str.substr(0, found);
    if (str[found+1] == '{') {
      std::size_t end = str.find('}');
      if (end != -1) {
        component = str.substr(found+2, end-found-2); // Extract variable name
        // Look up environment variable and expand it
        if (getenv(component.c_str())) result += getenv(component.c_str());
        else result += component; // If not found, keep original text     
        str = str.substr(end+1,-1);
      }
    }
    found = str.find('$');
  }
  result += str;
  yylval.cpp_string = new std::string(result);
  return WORD;
}

\`[^\n\`]*\`|$\([^\n]*\) {
  std::string str = std::string(yytext);
  if (str.at(0) == '$') {
    str = str.substr(1,str.size()-1); // Remove $( and )
  }
  str = str.substr(1,str.size()-2); // Remove ` or ( and ` or )
  str += "\nexit\n"; // Ensure subshell exits properly

  // Set up pipes for communication with subshell (Part 2.8: use pipes, not popen/temp files)
  int pin[2], pout[2], tmpin, tmpout;
  tmpin = dup(0); tmpout = dup(1); // Save original stdin/stdout
  pipe(pin); pipe(pout); // Create input and output pipes

  // Redirect stdin/stdout to pipes for subshell communication
  dup2(pin[0], 0);   // Subshell reads from pin[0]
  dup2(pout[1], 1);  // Subshell writes to pout[1]
  close(pin[0]);
  close(pout[1]);

  // Send command to subshell through input pipe
  write(pin[1], str.c_str(), str.size());
  close(pin[1]);

  // Create subshell process
  int ret = fork();
  if (ret==0) {
    // Child process - execute my shell as subshell
    char ** args = new char*[2];
    args[0] = (char*)"/proc/self/exe"; // Execute same shell binary
    args[1] = NULL;
    execvp(args[0], args);
    perror("execvp(subshell)");
    exit(1);
  } else if (ret < 0) {
    perror("fork");
    exit(1);
  } else {
    // Parent process - waiting for subshell to complete
    waitpid(ret, NULL, 0);

    // Restore original stdin/stdout
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    close(tmpin);
    close(tmpout);
  
    // Read output from subshell
    int BUFSIZE = 1024;
    char* c = new char[BUFSIZE];
    int rd = read(pout[0], c, BUFSIZE);
    close(pout[0]);

    // Insert subshell output back into input stream in reverse order
    for (int i = rd - 12; i >= 0; --i ) {
      if (c[i] == '\n') {
        c[i] = '\t'; // Convert newlines to tabs for argument separation
      }
      myunputc( c[i] ); // Put characters back into input stream
    }
    delete c;
  }
}

[^ \|>\t\n][^ \|>\t\n]*  {
  /* Basic word recognition for commands, arguments, and filenames */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
