
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

/* 
 * Sets terminal into raw mode. 
 * This causes having the characters available
 * immediately instead of waiting for a newline. 
 * Also there is no automatic echo.
 */
static struct termios saved_attr;
static int attr_saved = 0;

void tty_raw_mode(void)
{
  struct termios tty_attr;

  // Save original only once
  if (!attr_saved) {
    tcgetattr(0, &saved_attr);  // save original terminal settings
    attr_saved = 1;
  }

  tcgetattr(0,&tty_attr);

  /* Set raw mode. */
  tty_attr.c_lflag &= (~(ICANON|ECHO));
  tty_attr.c_cc[VTIME] = 0;
  tty_attr.c_cc[VMIN] = 1;

  tcsetattr(0,TCSANOW,&tty_attr);
}

void tty_restore_mode(void)
{
  if (attr_saved) {
    tcsetattr(0, TCSANOW, &saved_attr);  // restore original terminal state
  }
}

