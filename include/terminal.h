#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <esc_seq.h>

// represent mutable terminal grid and parser context
typedef struct TerminalState {
    int *grid;
    float text_scale;
    bool cursor_visible;
    double last_toggle;
    double last_input_time;
    int cursor_row;
    int cursor_col;
    int scroll_top;
    int scroll_bottom;
    esc_parser_t parser;
} TerminalState;

// initialize terminal grid and parser resources
int terminal_init(TerminalState *term, float initial_scale);
// release any terminal resources
void terminal_free(TerminalState *term);
// adjust terminal to match new pixel dimensions
bool terminal_resize(TerminalState *term, int width, int height);
// process bytes read from pty and update grid
void terminal_process_data(TerminalState *term, const uint8_t *data, size_t len);
// record latest input activity timestamp
void terminal_on_input_activity(TerminalState *term, double now);
// refresh cursor blink state for current frame
void terminal_update_cursor(TerminalState *term, double now);
// render current grid contents and cursor
void terminal_render(const TerminalState *term, GLuint shader_program, const vec3 fg_color, const vec3 bg_color);

#endif // TERMINAL_H

