
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <sys/types.h>
#include <string>
#include <string.h>
#include <regex.h>     // For wildcard expansion (Part 3.3: Wildcarding)
#include <dirent.h>    // For directory operations in wildcard expansion
#include <algorithm>   // For sorting wildcard matches
#include <sys/stat.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
#define MAXFILENAME 1024
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

// Token definitions for shell grammar (Part 1A: Lex and Yacc)
%token <cpp_string> WORD   // Commands, arguments, filenames
%token NOTOKEN GREAT NEWLINE LESS PIPE AMPERSAND GREATGREAT GREATAND TWOGREAT GREATGREATAND

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"
#include <string>

void yyerror(const char * s);
void expandWildcardsIfNecessary(std::string * arg);  // Part 3.3: Wildcarding helper
void expandWildcard(char * prefix, char * suffix);   // Part 3.3: Recursive wildcard expansion
bool cmpfunction (char * i, char * j);               // Comparison function for sorting

int yylex();
// Global variables for wildcard processing (Part 3.3: Wildcarding)
static std::vector<char *> _sortArgument = std::vector<char *>();  // Collects wildcard matches
static bool wildCard;  // Flag indicating if wildcards were found
%}

%%

// Top-level grammar rule - shell accepts multiple commands
goal:
  commands
  ;

// Multiple commands can be entered
commands:
  command
  | commands command
  ;

// Command structure (Part 1A: Extended grammar)
// Supports: pipes, I/O redirection, and background execution
command: 
  pipe_list iomodifier_list background_opt NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();  // Execute the parsed command
  }
  | NEWLINE {
    // Empty command line - just show prompt again
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | error NEWLINE { yyerrok; }  // Error recovery - continue parsing after errors
  ;

// Simple command structure - command with arguments
simple_command:	
  command_and_args 
  ;

// Build a complete simple command from command word and arguments
command_and_args:
  command_word argument_list {    
    // Add this simple command to the current command's pipeline
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

// Argument list can be empty or contain multiple arguments
argument_list:
  argument_list argument
  | /* can be empty */
  ;

// Process each argument with wildcard expansion (Part 3.3: Wildcarding)
argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    //Command::_currentSimpleCommand->insertArgument( $1 );
    
    // Wildcard expansion: * matches 0+ chars, ? matches 1 char
    // Wildcards don't match . if it's the first character (hidden files)
    wildCard = false;
    char *p = (char *)"";  // Empty prefix to start
    expandWildcard(p, (char *)$1->c_str());  // Recursively expand wildcards
    
    // Sort wildcard matches alphabetically as required
    std::sort(_sortArgument.begin(), _sortArgument.end(), cmpfunction);
    
    // Insert all expanded arguments
    for (auto a: _sortArgument) {
      std::string * argToInsert = new std::string(a);
      Command::_currentSimpleCommand->insertArgument(argToInsert);
    }
    _sortArgument.clear();  // Clean up for next argument
    //if (!wildCard) Command::_currentSimpleCommand->insertArgument($1);
  }
  ;

// First word of a command - this creates a new SimpleCommand
command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    
    // Create new simple command and insert the command name as first argument
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

// Pipeline support (Part 1B.3: Pipes)
// Supports multiple commands connected by pipes: cmd1 | cmd2 | cmd3
pipe_list:
  pipe_list PIPE simple_command   // Add another command to the pipeline
  | simple_command               // Start of pipeline (single command)
  ;

// I/O redirection options (Part 1B.2: File redirection)
// Supports all redirection operators from the extended grammar
iomodifier_opt:
  GREAT WORD {
    // Simple output redirection: command > file
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);  // 1 = stdout
  }
  | GREATGREAT WORD {
    // Append output redirection: command >> file
    //printf("   Yacc: append output \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);
    Shell::_currentCommand._append = true;   // Set append flag
  }
  | GREATAND WORD {
    // Redirect both stdout and stderr: command >& file
    //printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);  // stdout
    Shell::_currentCommand.redirect(2, new std::string($2->c_str()));  // stderr
    
  }
  | GREATGREATAND WORD {
    // Append both stdout and stderr: command >>& file
    //printf("   Yacc: append output and error \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(1,$2);  // stdout
    Shell::_currentCommand.redirect(2, new std::string($2->c_str()));  // stderr
    Shell::_currentCommand._append = true;
  }
  | LESS WORD {
    // Input redirection: command < file
    //printf("   Yacc: get input \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(0, $2);  // 0 = stdin
  }
  | TWOGREAT WORD {
    // Error redirection: command 2> file
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    Shell::_currentCommand.redirect(2, $2);  // 2 = stderr
  }
  ;

// Multiple I/O redirections are allowed (but checked for ambiguity in redirect())
iomodifier_list:
  iomodifier_list iomodifier_opt  // Multiple redirections
  | /*empty*/                     // No redirections
  ;

// Background execution support (Part 1A: Grammar supports & for background)
background_opt:
  AMPERSAND {
    // Set background flag - command will run in background
    //printf("   Yacc: run in background\n");
    Shell::_currentCommand._background = true;
  }
  | /*empty*/  // Foreground execution (default)
  ;
%%

// Helper function for sorting wildcard matches alphabetically
bool cmpfunction (char * i, char * j) { return strcmp(i,j)<0; }

// Error handler for parser errors
void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

// Wildcard expansion function (Part 3.3: Wildcarding)
// This function is currently unused but shows alternative approach to wildcard expansion
void expandWildcardsIfNecessary(std::string * arg) {
  char * arg_c = (char *)arg->c_str();
  char * a;
  std::string path;
  
  // Check if argument contains wildcard characters
  if (strchr(arg_c,'?')==NULL & strchr(arg_c,'*')==NULL) {
    //printf("No '?' or '*' was found\n");
    Command::_currentSimpleCommand->insertArgument(arg);
    return;
  }
  
  // Handle absolute vs relative paths
  DIR * dir;
  if (arg_c[0] == '/') {
    // Absolute path - find the directory part
    std::size_t found = arg->find('/');
    while (arg->find('/',found+1) != -1) 
      found = arg->find('/', found+1);
      
    path = arg->substr(0, found+1);
    a = (char *)arg->substr(found+1, -1).c_str();
    dir = opendir(path.c_str());
    //printf("%s\n", path.c_str());
  }
  else {
    // Relative path - use current directory
    dir = opendir(".");
    a = arg_c;
  }
  if (dir == NULL) {
    perror("opendir");
    return;
  }
  
  // Convert shell wildcards to regular expression
  // * becomes .* (match any chars), ? becomes . (match one char)
  char * reg = (char*)malloc(2*strlen(arg_c)+10);
  char * r = reg;
  *r = '^'; r++;  // Start of string anchor
  while (*a) {
    if (*a == '*') {*r='.'; r++; *r='*'; r++;}      // * -> .*
    else if (*a == '?') {*r='.'; r++;}              // ? -> .
    else if (*a == '.') {*r='\\'; r++; *r='.'; r++;} // Escape literal dots
    else {*r=*a; r++;}                              // Regular characters
    a++;
  }
  *r='$'; r++; *r=0;  // End of string anchor

  // Compile the regular expression
  regex_t re;
  int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
  if (expbuf != 0) {
    perror("regcomp");
    return;
  }

  // Find all matching files in the directory
  std::vector<char *> sortArgument = std::vector<char *>();
  struct dirent * ent;
  while ( (ent=readdir(dir)) != NULL) {
    if (regexec(&re, ent->d_name, 1, NULL, 0) == 0) {
      // Check for hidden files (starting with .)
      if (reg[1] == '.') {
        // Pattern starts with . - only match if filename starts with .
        if (ent->d_name[0] != '.') {
          std::string name(ent->d_name);
          name = path + name;
          sortArgument.push_back(strdup((char *)name.c_str()));
        }
      } else {
        // Pattern doesn't start with . - don't match hidden files
        std::string name(ent->d_name);
        name = path + name;
        sortArgument.push_back(strdup((char *)name.c_str()));
      }
    }
  }

  closedir(dir);
  regfree(&re);

  // Sort matches alphabetically 
  std::sort(sortArgument.begin(), sortArgument.end(), cmpfunction);
  
  // Add all matches as arguments
  for (auto a: sortArgument) {
    std::string * argToInsert = new std::string(a);
    Command::_currentSimpleCommand->insertArgument(argToInsert);
  }

  sortArgument.clear();
}

// Main wildcard expansion function used by parser (Part 3.3: Wildcarding)
// Recursively processes path components to handle wildcards in any directory level
void expandWildcard(char * prefix, char * suffix) {
  // Base case: no more path components to process
  if (suffix[0] == 0) {
    //_sortArgument.push_back(strdup(prefix));
    // Normalize to remove trailing slash
  char cleaned[MAXFILENAME];
  strcpy(cleaned, prefix);
  int len = strlen(cleaned);
  if (len > 1 && cleaned[len - 1] == '/') {
    cleaned[len - 1] = '\0';
  }
  _sortArgument.push_back(strdup(cleaned));
    return;
  }
  
  // Build the current directory prefix
  char Prefix[MAXFILENAME];
  if (prefix[0] == 0) {
    if (suffix[0] == '/') {suffix += 1; sprintf(Prefix, "%s/", prefix);}
    else strcpy(Prefix, prefix);
  }
  else
    sprintf(Prefix, "%s/", prefix);

  // Extract the next path component
  char * s = strchr(suffix, '/');
  char component[MAXFILENAME];
  if (s != NULL) {
    // More components after this one
    strncpy(component, suffix, s-suffix);
    component[s-suffix] = 0;
    suffix = s + 1;
  }
  else {
    // This is the last component
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }

  char newPrefix[MAXFILENAME];
  // If component has no wildcards, just add it to path and continue
  if (strchr(component,'?')==NULL & strchr(component,'*')==NULL) {
    if (Prefix[0] == 0) strcpy(newPrefix, component);
    else sprintf(newPrefix, "%s/%s", prefix, component);
    expandWildcard(newPrefix, suffix);
    return;
  }
  
  // Component has wildcards - convert to regex and match against directory entries
  char * reg = (char*)malloc(2*strlen(component)+10);
  char * r = reg;
  *r = '^'; r++;  // Start anchor
  int i = 0;
  while (component[i]) {
    if (component[i] == '*') {*r='.'; r++; *r='*'; r++;}        // * -> .*
    else if (component[i] == '?') {*r='.'; r++;}               // ? -> .
    else if (component[i] == '.') {*r='\\'; r++; *r='.'; r++;} // Escape dots
    else {*r=component[i]; r++;}                               // Regular chars
    i++;
  }
  *r='$'; r++; *r=0;  // End anchor

  // Compile the regex
  regex_t re;
  int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
  
  // Open the directory to search
  char * dir;
  if (Prefix[0] == 0) dir = (char*)"."; else dir = Prefix;
  DIR * d = opendir(dir);
  if (d == NULL) {
    return;  // Directory doesn't exist
  }
  struct dirent * ent;
  bool find = false;
  // Check each directory entry against the pattern
  while ((ent = readdir(d)) != NULL) {
    if(regexec(&re, ent->d_name, 1, NULL, 0) == 0) {
      wildCard = true;
      find = true;
      if (Prefix[0] == 0) strcpy(newPrefix, ent->d_name);
      else sprintf(newPrefix, "%s/%s", prefix, ent->d_name);

      // Handle hidden files (starting with .) - only match if pattern explicitly starts with .
      if (reg[1] == '.') {
        if (ent->d_name[0] != '.') expandWildcard(newPrefix, suffix);
      } else 
        expandWildcard(newPrefix, suffix);
    }
  }
  
  // If no matches found, keep the original component (literal interpretation)
  /*if (!find && suffix[0] == '\0') {
    if (Prefix[0] == 0) {
      strcpy(newPrefix, component);
    }
    else {
      sprintf(newPrefix, "%s/%s", prefix, component);
    }
    expandWildcard(newPrefix, suffix);
    //_sortArgument.push_back(strdup(newPrefix));
  }*/

  // Final fallback: only for the last component if no match
  if (!find && suffix[0] == '\0' && !(strchr(component, '*') || strchr(component, '?'))) {

    // Only allow fallback for actual command-line words, not intermediate path segments
    // fallback: treat as literal if no matches
      if (Prefix[0] == 0) {
        strcpy(newPrefix, component);
      } else {
        sprintf(newPrefix, "%s/%s", prefix, component);
      }
      _sortArgument.push_back(strdup(newPrefix));
    }






  // Cleanup
  closedir(d);
  regfree(&re);
  free(reg);
}

#if 0
main()
{
  yyparse();
}
#endif
