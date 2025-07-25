#ifndef shell_hh
#define shell_hh

#include "command.hh"
#include <vector>

#ifndef YY_BUF_SIZE
#ifdef __ia64__
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

// These are declared in C++ in lex.yy.cc
extern void yyrestart(FILE * input_file);
extern void yypush_buffer_state(YY_BUFFER_STATE buffer);
extern void yypop_buffer_state(void);
extern YY_BUFFER_STATE yy_create_buffer(FILE * file, int size);
extern int yyparse(void); // <-- include this too


struct Shell {

  static void prompt();

  static Command _currentCommand;

  static std::vector<int> _bgPIDs;

  static bool _srcCmd;
};

#endif
