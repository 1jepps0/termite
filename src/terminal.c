#include <terminal.h>

#include <stdlib.h>

#include <text.h>

#define CURSOR_BLINK_INTERVAL 0.5
#define CURSOR_INPUT_PAUSE 0.15

static void terminal_handle_control_char(TerminalState *term, uint8_t byte);

int terminal_init(TerminalState *term, float initial_scale) {
    if (!term)
        return -1;

    // reset terminal fields to defaults
    term->grid = NULL;
    term->text_scale = initial_scale;
    term->cursor_visible = true;
    term->last_toggle = 0.0;
    term->last_input_time = 0.0;
    term->cursor_row = 0;
    term->cursor_col = 0;
    term->scroll_top = 0;
    term->scroll_bottom = 0;
    esc_parser_init(&term->parser);

    // configure base metrics before allocating grid
    text_set_base_scale(term->text_scale);
    x_spacing = glyph_width * term->text_scale;
    y_spacing = glyph_height * term->text_scale;

    // allocate backing grid filled with spaces
    term->grid = text_setup_grid();
    if (!term->grid)
        return -1;

    term->scroll_top = 0;
    term->scroll_bottom = grid_y_size - 1;
    return 0;
}

void terminal_free(TerminalState *term) {
    if (!term)
        return;
    // release dynamic grid buffer
    free(term->grid);
    term->grid = NULL;
}

bool terminal_resize(TerminalState *term, int width, int height) {
    if (!term || !term->grid)
        return false;

    // rebuild grid to match new resolution
    int *new_grid = text_resize_grid(term->grid, width, height, &term->text_scale, &term->cursor_row,
                               &term->cursor_col, &term->scroll_top, &term->scroll_bottom);
    if (!new_grid)
        return false;

    term->grid = new_grid;
    return true;
}

void terminal_process_data(TerminalState *term, const uint8_t *data, size_t len) {
    if (!term || !term->grid || !data)
        return;

    // parse escape sequences and printable bytes
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];

        if (esc_parser_process(&term->parser, byte, &term->cursor_row, &term->cursor_col, term->grid,
                               grid_x_size, grid_y_size, &term->scroll_top, &term->scroll_bottom)) {
            continue;
        }

        terminal_handle_control_char(term, byte);
    }
}

void terminal_on_input_activity(TerminalState *term, double now) {
    if (!term)
        return;
    // keep cursor visible while receiving data
    term->cursor_visible = true;
    term->last_toggle = now;
    term->last_input_time = now;
}

void terminal_update_cursor(TerminalState *term, double now) {
    if (!term)
        return;

    // toggle cursor when idle 
    if (now - term->last_input_time < CURSOR_INPUT_PAUSE) {
        term->cursor_visible = true;
        term->last_toggle = now;
    } else if (now - term->last_toggle >= CURSOR_BLINK_INTERVAL) {
        term->cursor_visible = !term->cursor_visible;
        term->last_toggle = now;
    }
}

void terminal_render(const TerminalState *term, GLuint shader_program, const vec3 fg_color,
                     const vec3 bg_color) {
    if (!term || !term->grid)
        return;

    // draw each glyph and cursor overlay
    for (int y = 0; y < grid_y_size; y++) {
        int draw_y = grid_y_size - 1 - y;

        for (int x = 0; x < grid_x_size; x++) {
            char c = (char)term->grid[y * grid_x_size + x];
            float xpos = margin_x + x * x_spacing;
            float ypos = margin_y * 1.25f + draw_y * y_spacing;

            if (term->cursor_visible && y == term->cursor_row && x == term->cursor_col) {
                text_render_cursor(shader_program, term->text_scale, draw_y, x, fg_color, bg_color, c);
            } else {
                text_render_char(shader_program, c, xpos, ypos, term->text_scale, fg_color);
            }
        }
    }
}

static void terminal_handle_control_char(TerminalState *term, uint8_t byte) {
    switch (byte) {
    case '\r':
        // return carriage to column zero
        term->cursor_col = 0;
        break;
    case '\n':
        term->cursor_col = 0;
        term->cursor_row++;
        if (term->cursor_row > term->scroll_bottom) {
            // scroll region upward when leaving bottom
            term_scroll_region_up(term->grid, grid_x_size, term->scroll_top, term->scroll_bottom);
            term->cursor_row = term->scroll_bottom;
        }
        break;
    case '\b':
        if (term->cursor_col > 0)
            term->cursor_col--;
        break;
    case '\x7F':
        if (term->cursor_col > 0) {
            term->cursor_col--;
            term->grid[term->cursor_row * grid_x_size + term->cursor_col] = ' ';
        }
        break;
    default:
        if (byte >= 32 && byte <= 126) {
            // place printable character and advance cursor
            term->grid[term->cursor_row * grid_x_size + term->cursor_col] = byte;
            term->cursor_col++;
            if (term->cursor_col >= grid_x_size) {
                term->cursor_col = 0;
                term->cursor_row++;
                if (term->cursor_row > term->scroll_bottom) {
                    // scroll when reaching end of region
                    term_scroll_region_up(term->grid, grid_x_size, term->scroll_top, term->scroll_bottom);
                    term->cursor_row = term->scroll_bottom;
                }
            }
        }
        break;
    }
}

