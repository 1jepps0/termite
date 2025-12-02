#include <esc_seq.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

// move scroll region upward by one line
void term_scroll_region_up(int *grid, int grid_x_size, int scroll_top, int scroll_bottom) {
    if (scroll_top >= scroll_bottom)
        return;
    int height = scroll_bottom - scroll_top;
    memmove(grid + scroll_top * grid_x_size,
            grid + (scroll_top + 1) * grid_x_size,
            sizeof(int) * grid_x_size * height);
    memset(grid + scroll_bottom * grid_x_size, ' ', sizeof(int) * grid_x_size);
}

// move scroll region downward by one line
void term_scroll_region_down(int *grid, int grid_x_size, int scroll_top, int scroll_bottom) {
    if (scroll_top >= scroll_bottom)
        return;
    int height = scroll_bottom - scroll_top;
    memmove(grid + (scroll_top + 1) * grid_x_size,
            grid + scroll_top * grid_x_size,
            sizeof(int) * grid_x_size * height);
    memset(grid + scroll_top * grid_x_size, ' ', sizeof(int) * grid_x_size);
}

void esc_parser_init(esc_parser_t *parser) {
    // reset parser buffers and state
    parser->state = ESC_STATE_NORMAL;
    parser->buf_pos = 0;
    parser->param_count = 0;
    parser->osc_waiting_backslash = false;
    parser->params_present = false;
    memset(parser->buf, 0, sizeof(parser->buf));
    memset(parser->params, 0, sizeof(parser->params));
}

static int parse_int(const char *str, size_t len, int *out) {
    int val = 0;
    size_t i = 0;
    while (i < len && isdigit((unsigned char)str[i])) {
        val = val * 10 + (str[i] - '0');
        i++;
    }
    *out = val;
    return i;
}

static void parse_params(esc_parser_t *parser) {
    // extract numeric parameters separated by semicolons
    parser->param_count = 0;
    parser->params_present = false;
    size_t i = 0;
    while (i < parser->buf_pos && parser->param_count < 16) {
        if (isdigit((unsigned char)parser->buf[i])) {
            int val;
            size_t consumed = parse_int(parser->buf + i, parser->buf_pos - i, &val);
            parser->params[parser->param_count++] = val;
            parser->params_present = true;
            i += consumed;
        } else if (parser->buf[i] == ';') {
            i++;
        } else {
            break;
        }
    }
    if (!parser->params_present) {
        parser->params[0] = 1;
        parser->param_count = 1;
    }
}

// handle state machine for escape sequence parsing
int esc_parser_process(esc_parser_t *parser, uint8_t byte, int *cursor_row, int *cursor_col, int *grid, int grid_x_size, int grid_y_size, int *scroll_top, int *scroll_bottom) {
    switch (parser->state) {
    case ESC_STATE_NORMAL:
        parser->osc_waiting_backslash = false;
        if (byte == 0x1b) {
            // begin escape sequence detection
            parser->state = ESC_STATE_ESC;
            parser->buf_pos = 0;
            return 1;
        }
        return 0;

    case ESC_STATE_ESC:
        if (parser->osc_waiting_backslash) {
            if (byte == '\\') {
                // terminate osc sequence
                parser->state = ESC_STATE_NORMAL;
                parser->buf_pos = 0;
                parser->osc_waiting_backslash = false;
                return 1;
            } else {
                parser->osc_waiting_backslash = false;
            }
        }
        if (byte == '[') {
            // start csi sequence
            parser->state = ESC_STATE_CSI;
            parser->buf_pos = 0;
            return 1;
        } else if (byte == ']') {
            // start osc sequence
            parser->state = ESC_STATE_OSC;
            parser->buf_pos = 0;
            return 1;
        } else if (byte == 'D') {
            // index escape declass cursor down
            if (*cursor_row >= *scroll_top && *cursor_row <= *scroll_bottom) {
                if (*cursor_row == *scroll_bottom) {
                    term_scroll_region_up(grid, grid_x_size, *scroll_top, *scroll_bottom);
                } else if (*cursor_row < *scroll_bottom) {
                    (*cursor_row)++;
                }
            } else if (*cursor_row < grid_y_size - 1) {
                (*cursor_row)++;
            }
            parser->state = ESC_STATE_NORMAL;
            parser->buf_pos = 0;
            return 1;
        } else if (byte == 'M') {
            // reverse index escape declass cursor up
            if (*cursor_row >= *scroll_top && *cursor_row <= *scroll_bottom) {
                if (*cursor_row == *scroll_top) {
                    term_scroll_region_down(grid, grid_x_size, *scroll_top, *scroll_bottom);
                } else if (*cursor_row > *scroll_top) {
                    (*cursor_row)--;
                }
            } else if (*cursor_row > 0) {
                (*cursor_row)--;
            }
            parser->state = ESC_STATE_NORMAL;
            parser->buf_pos = 0;
            return 1;
        } else if (byte == 'E') {
            // next line escape moves cursor and resets column
            if (*cursor_row >= *scroll_top && *cursor_row <= *scroll_bottom) {
                if (*cursor_row == *scroll_bottom) {
                    term_scroll_region_up(grid, grid_x_size, *scroll_top, *scroll_bottom);
                } else if (*cursor_row < *scroll_bottom) {
                    (*cursor_row)++;
                }
            } else if (*cursor_row < grid_y_size - 1) {
                (*cursor_row)++;
            }
            *cursor_col = 0;
            parser->state = ESC_STATE_NORMAL;
            parser->buf_pos = 0;
            return 1;
        } else {
            parser->state = ESC_STATE_NORMAL;
            return 0;
        }

    case ESC_STATE_OSC:
        if (byte == 0x07) {
            // bel terminates osc
            parser->state = ESC_STATE_NORMAL;
            parser->buf_pos = 0;
            parser->osc_waiting_backslash = false;
            return 1;
        } else if (byte == 0x1b) {
            // esc expects final backslash
            parser->osc_waiting_backslash = true;
            parser->state = ESC_STATE_ESC;
            return 1;
        } else {
            return 1;
        }

    case ESC_STATE_CSI:
        if (byte >= 0x40 && byte <= 0x7E) {
            char cmd = (char)byte;
            parse_params(parser);

            int n = parser->params[0];

            switch (cmd) {
            case 'A':
                // cursor up csi n a
                *cursor_row = (*cursor_row - n) < 0 ? 0 : (*cursor_row - n);
                break;
            case 'B':
                // cursor down csi n b
                *cursor_row = (*cursor_row + n) >= grid_y_size ? (grid_y_size - 1) : (*cursor_row + n);
                break;
            case 'C':
                // cursor forward csi n c
                *cursor_col = (*cursor_col + n) >= grid_x_size ? (grid_x_size - 1) : (*cursor_col + n);
                break;
            case 'D':
                // cursor backward csi n d
                *cursor_col = (*cursor_col - n) < 0 ? 0 : (*cursor_col - n);
                break;
            case 'H':
            case 'f':
                {
                    // cursor position csi row col h or f
                    int row = parser->param_count > 0 ? parser->params[0] : 1;
                    int col = parser->param_count > 1 ? parser->params[1] : 1;
                    *cursor_row = (row - 1) < 0 ? 0 : ((row - 1) >= grid_y_size ? (grid_y_size - 1) : (row - 1));
                    *cursor_col = (col - 1) < 0 ? 0 : ((col - 1) >= grid_x_size ? (grid_x_size - 1) : (col - 1));
                }
                break;
            case 'J':
                {
                    // erase in display csi n j
                    int mode = parser->params_present ? n : 0;
                    if (mode == 0 || mode == 1) {
                        for (int y = mode == 0 ? *cursor_row : 0;
                             y <= (mode == 0 ? grid_y_size - 1 : *cursor_row);
                             y++) {
                            int start_col = (y == *cursor_row) ? (mode == 0 ? *cursor_col : 0) : 0;
                            int end_col = (y == *cursor_row) ? (mode == 0 ? grid_x_size : *cursor_col + 1) : grid_x_size;
                            for (int x = start_col; x < end_col; x++) {
                                grid[y * grid_x_size + x] = ' ';
                            }
                        }
                    } else if (mode == 2) {
                        memset(grid, ' ', sizeof(int) * grid_x_size * grid_y_size);
                    }
                }
                break;
            case 'K':
                {
                    // erase in line csi n k
                    int mode = parser->params_present ? n : 0;
                    int y = *cursor_row;
                    int start_col, end_col;
                    if (mode == 0) {
                        start_col = *cursor_col;
                        end_col = grid_x_size;
                    } else if (mode == 1) {
                        start_col = 0;
                        end_col = *cursor_col + 1;
                    } else {
                        start_col = 0;
                        end_col = grid_x_size;
                    }
                    for (int x = start_col; x < end_col; x++) {
                        grid[y * grid_x_size + x] = ' ';
                    }
                }
                break;
            case 'L':
                {
                    // insert line csi n l
                    if (*cursor_row < *scroll_top || *cursor_row > *scroll_bottom)
                        break;
                    int count = parser->params_present ? n : 1;
                    if (count < 0)
                        count = 0;
                    int start = *cursor_row;
                    int limit = *scroll_bottom - start + 1;
                    if (limit <= 0)
                        break;
                    if (count > limit)
                        count = limit;
                    int move = limit - count;
                    if (move > 0) {
                        memmove(grid + (start + count) * grid_x_size,
                                grid + start * grid_x_size,
                                sizeof(int) * grid_x_size * move);
                    }
                    for (int i = 0; i < count; i++) {
                        memset(grid + (start + i) * grid_x_size, ' ', sizeof(int) * grid_x_size);
                    }
                }
                break;
            case 'M':
                {
                    // delete line csi n m
                    if (*cursor_row < *scroll_top || *cursor_row > *scroll_bottom)
                        break;
                    int count = parser->params_present ? n : 1;
                    if (count < 0)
                        count = 0;
                    int start = *cursor_row;
                    int limit = *scroll_bottom - start + 1;
                    if (limit <= 0)
                        break;
                    if (count > limit)
                        count = limit;
                    int move = limit - count;
                    if (move > 0) {
                        memmove(grid + start * grid_x_size,
                                grid + (start + count) * grid_x_size,
                                sizeof(int) * grid_x_size * move);
                    }
                    for (int i = 0; i < count; i++) {
                        memset(grid + (*scroll_bottom - i) * grid_x_size, ' ', sizeof(int) * grid_x_size);
                    }
                }
                break;
            case 'S':
                {
                    // scroll up csi n s
                    int count = parser->params_present ? n : 1;
                    if (count < 0)
                        count = 0;
                    int limit = *scroll_bottom - *scroll_top + 1;
                    if (limit <= 0)
                        break;
                    if (count > limit)
                        count = limit;
                    for (int i = 0; i < count; i++)
                        term_scroll_region_up(grid, grid_x_size, *scroll_top, *scroll_bottom);
                }
                break;
            case 'T':
                {
                    // scroll down csi n t
                    int count = parser->params_present ? n : 1;
                    if (count < 0)
                        count = 0;
                    int limit = *scroll_bottom - *scroll_top + 1;
                    if (limit <= 0)
                        break;
                    if (count > limit)
                        count = limit;
                    for (int i = 0; i < count; i++)
                        term_scroll_region_down(grid, grid_x_size, *scroll_top, *scroll_bottom);
                }
                break;
            case 'r':
                {
                    // set scroll region csi top bottom r
                    if (!parser->params_present) {
                        *scroll_top = 0;
                        *scroll_bottom = grid_y_size - 1;
                    } else {
                        int top = parser->params[0] > 0 ? parser->params[0] - 1 : 0;
                        int bottom = (parser->param_count > 1) ? parser->params[1] - 1 : (grid_y_size - 1);
                        if (top < 0)
                            top = 0;
                        if (top >= grid_y_size)
                            top = grid_y_size - 1;
                        if (bottom < 0)
                            bottom = 0;
                        if (bottom >= grid_y_size)
                            bottom = grid_y_size - 1;
                        if (top >= bottom) {
                            *scroll_top = 0;
                            *scroll_bottom = grid_y_size - 1;
                        } else {
                            *scroll_top = top;
                            *scroll_bottom = bottom;
                        }
                    }
                    if (*cursor_row < *scroll_top)
                        *cursor_row = *scroll_top;
                    if (*cursor_row > *scroll_bottom)
                        *cursor_row = *scroll_bottom;
                    *cursor_col = 0;
                }
                break;
            case 'm':
                // select graphic rendition csi parameters m
                break;
            default:
                break;
            }

            parser->state = ESC_STATE_NORMAL;
            parser->buf_pos = 0;
            return 1;
        } else if (byte >= 0x20 && byte <= 0x3F) {
            if (parser->buf_pos < sizeof(parser->buf) - 1) {
                parser->buf[parser->buf_pos++] = byte;
            }
            return 1;
        } else {
            parser->state = ESC_STATE_NORMAL;
            parser->buf_pos = 0;
            return 0;
        }

    default:
        parser->state = ESC_STATE_NORMAL;
        parser->buf_pos = 0;
        return 0;
    }
}
