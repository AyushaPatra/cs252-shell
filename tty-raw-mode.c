
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/*
 * Sets terminal into raw mode. 
 * This causes having the characters available
 * immediately instead of waiting for a newline. 
 * Also there is no automatic echo.
 */

static struct termios original_termios;

void tty_reset_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

void tty_raw_mode(void)
{
  struct termios tty_attr;
  tcgetattr(0,&tty_attr);

  // Save original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);

  // Register reset function to be called at exit
  atexit(tty_reset_mode);

  /* Set raw mode. */
  tty_attr = original_termios;
  tty_attr.c_lflag &= ~(ICANON | ECHO);
  tty_attr.c_cc[VTIME] = 0;
  tty_attr.c_cc[VMIN] = 1;

  tcsetattr(STDIN_FILENO, TCSANOW, &tty_attr);
}
