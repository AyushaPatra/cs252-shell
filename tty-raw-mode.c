#include <termios.h>
#include <unistd.h>

static struct termios original_termios;

void tty_raw_mode(void) {
    struct termios tty_attr;

    // Save original attributes
    tcgetattr(STDIN_FILENO, &original_termios);

    // Register restore at exit
    atexit(tty_reset_mode);

    // Set raw mode
    tty_attr = original_termios;
    tty_attr.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
    tty_attr.c_cc[VMIN] = 1;
    tty_attr.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty_attr);
}

void tty_reset_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}
