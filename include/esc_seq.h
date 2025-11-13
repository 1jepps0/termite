#ifndef ESC_SEQ_H
#define ESC_SEQ_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    ESC_STATE_NORMAL,
    ESC_STATE_ESC,
    ESC_STATE_CSI,
    ESC_STATE_CSI_PARAM,
    ESC_STATE_OSC
} esc_state_t;

typedef struct {
    esc_state_t state;
    char buf[256];
    size_t buf_pos;
    int params[16];
    int param_count;
    bool osc_waiting_backslash;
    bool params_present;
} esc_parser_t;

void esc_parser_init(esc_parser_t *parser);
int esc_parser_process(esc_parser_t *parser, uint8_t byte, int *cursor_row, int *cursor_col, int *grid, int grid_x_size, int grid_y_size, int *scroll_top, int *scroll_bottom);
void term_scroll_region_up(int *grid, int grid_x_size, int scroll_top, int scroll_bottom);
void term_scroll_region_down(int *grid, int grid_x_size, int scroll_top, int scroll_bottom);

#endif
