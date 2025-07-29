#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;

  bool _append;

  bool _clear;
  int _lastPID;
  int _lastRT;
  std::string getLastArg();

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  static SimpleCommand *_currentSimpleCommand;
};

#endif
