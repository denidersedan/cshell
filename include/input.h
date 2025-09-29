#ifndef INPUT_H
#define INPUT_H

/* initialize input layer (call once on startup) */
void input_init(void);

/* restore terminal to original state (call on exit or use atexit). */
void input_restore(void);

/* Read a line with prompt (supports arrow-up/down history navigation).
 * Returns a malloc'd string (caller must free). Returns NULL on EOF (Ctrl-D on empty line). */
char *read_input_line(const char *prompt);

#endif
