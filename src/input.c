/* src/input.c */
#define _POSIX_C_SOURCE 200809L
#include "input.h"
#include "history.h"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define INPUT_BUFSIZE 4096

static struct termios orig_termios;
static int raw_enabled = 0;

// restore terminal to canonical mode
void input_restore(void) {
    if (raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_enabled = 0;
    }
}

// enable raw-ish mode (disable canonical mode + echo)
void input_init(void) {
    if (raw_enabled) return;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return;
    }
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // turn off canonical mode and echo
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return;
    }
    raw_enabled = 1;
    atexit(input_restore);
}

// write all bytes
static ssize_t xwrite(const void *buf, size_t n) {
    size_t left = n;
    const char *p = buf;
    while (left > 0) {
        ssize_t w = write(STDOUT_FILENO, p, left);
        if (w <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= w;
        p += w;
    }
    return (ssize_t)n;
}

// redraw prompt + buffer (clear line then print)
static void redraw_line(const char *prompt, const char *buf, size_t len) {
    // \r = carriage return, \x1b[K clears to end of line
    xwrite("\r", 1);
    xwrite("\x1b[K", 3);
    xwrite(prompt, strlen(prompt));
    if (len > 0) xwrite(buf, len);
}

static int read_char(void) {
    unsigned char c;
    ssize_t n;
    
    // Loop until we get a real read or a real error
    do {
        n = read(STDIN_FILENO, &c, 1);
    } while (n == -1 && errno == EINTR);

    if (n == 1) return c;
    if (n == 0) return -1; // EOF
    return -1; // Other error
}

// main line reader with simple editing + history up/down arrows
char *read_input_line(const char *prompt) {
    if (!prompt) prompt = "$ ";

    // enable raw mode if not already
    if (!raw_enabled) input_init();

    // print prompt
    xwrite(prompt, strlen(prompt));

    char *buf = malloc(INPUT_BUFSIZE);
    if (!buf) return NULL;
    size_t buflen = 0;
    buf[0] = '\0';

    int hist_top = history_count(); // one past last
    int hist_pos = hist_top;        // current history position
    int done = 0;

    while (!done) {
        int c = read_char();
        if (c == -1) {
            // read error or EOF — treat as EOF if buffer empty
            if (buflen == 0) {
                free(buf);
                return NULL;
            } else {
                // treat as end-of-line
                break;
            }
        }

        if (c == 127 || c == 8) { // Backspace or DEL
            if (buflen > 0) {
                buflen--;
                buf[buflen] = '\0';
                redraw_line(prompt, buf, buflen);
            } else {
                // nothing to do
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            // newline: finish
            xwrite("\r\n", 2);
            done = 1;
            break;
        }

        if (c == 4) { // Ctrl-D (EOF) — if buffer empty -> return NULL to signal EOF; else ignore
            if (buflen == 0) {
                free(buf);
                return NULL;
            }
            continue;
        }

        if (c == 27) { // Escape sequence
            // Expect '[' and a final char like 'A','B','C','D'
            int c1 = read_char();
            int c2 = read_char();
            if (c1 == '[') {
                if (c2 == 'A') { // Up arrow
                    if (hist_pos > 0) {
                        hist_pos--;
                        const char *h = history_get(hist_pos);
                        if (h) {
                            size_t hlen = strlen(h);
                            if (hlen >= INPUT_BUFSIZE) hlen = INPUT_BUFSIZE - 1;
                            memcpy(buf, h, hlen);
                            buflen = hlen;
                            buf[buflen] = '\0';
                            redraw_line(prompt, buf, buflen);
                        }
                    }
                } else if (c2 == 'B') { // Down arrow
                    if (hist_pos < hist_top - 1) {
                        hist_pos++;
                        const char *h = history_get(hist_pos);
                        if (h) {
                            size_t hlen = strlen(h);
                            if (hlen >= INPUT_BUFSIZE) hlen = INPUT_BUFSIZE - 1;
                            memcpy(buf, h, hlen);
                            buflen = hlen;
                            buf[buflen] = '\0';
                            redraw_line(prompt, buf, buflen);
                        }
                    } else if (hist_pos == hist_top - 1) {
                        // move to new blank line
                        hist_pos = hist_top;
                        buflen = 0;
                        buf[0] = '\0';
                        redraw_line(prompt, buf, buflen);
                    }
                } else {
                    // left/right and others are ignored for now
                }
            } else {
                // not a standard CSI sequence; ignore
            }
            continue;
        }

        // printable character (we keep it simple — no cursor movement)
        if (c >= 32 && c < 127) {
            if (buflen + 1 < INPUT_BUFSIZE) {
                buf[buflen++] = (char)c;
                buf[buflen] = '\0';
                // echo the character
                char cc = (char)c;
                xwrite(&cc, 1);
            } else {
                // buffer full: ignore more input or beep
            }
        } else {
            // ignore other control codes
        }
    } // end read loop

    // return a copy (caller frees). If buffer empty return empty string (not NULL).
    char *ret = strdup(buf);
    free(buf);
    return ret;
}
