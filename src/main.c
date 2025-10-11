// built in libraries
#include <stdbool.h>
#include <stdint.h>

// installed libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <string.h>

// internal libraries
#include <pty_wrap.h>
#include <shader.h>
#include <text.h>
#include <window.h>

int master_fd;
int child_pid;

float text_scale = 0.35;
bool cursor_visible = true;
double last_toggle = 0.0;

int term_cursor_row = 0;
int term_cursor_col = 0;

extern int master_fd;

void char_callback(GLFWwindow *window, unsigned int codepoint) {
  char c = (char)codepoint;
  pty_write(master_fd, &c, 1);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (action == GLFW_PRESS) {
    switch (key) {
    case GLFW_KEY_ENTER:
      pty_write(master_fd, "\r", 1);
      break;
    case GLFW_KEY_BACKSPACE:
      pty_write(master_fd, "\x7F", 1);
      break;
    case GLFW_KEY_UP:
      pty_write(master_fd, "\x1b[A", 3);
      break;
    case GLFW_KEY_DOWN:
      pty_write(master_fd, "\x1b[B", 3);
      break;
    case GLFW_KEY_RIGHT:
      pty_write(master_fd, "\x1b[C", 3);
      break;
    case GLFW_KEY_LEFT:
      pty_write(master_fd, "\x1b[D", 3);
      break;
    }
  }
}

int main(void) {

  // setup window
  GLFWwindow *window = InitializeWindow();
  if (window == NULL) {
    return -1;
  }

  glfwSetCharCallback(window, char_callback);
  glfwSetKeyCallback(window, key_callback);

  // initialize freetype characters
  if (SetupCharacters() != 0) {
    return -1;
  }

  // set up shaders
  GLuint shaderProgram = initialize_shader();

  x_spacing = glyph_width * text_scale;
  y_spacing = glyph_height * text_scale;

  int *grid = SetupGrid();
  int baseline_y = ascent * text_scale;
  vec3 fg = {0.9, 0.9f, 1.0f};
  vec3 bg = {0.02f, 0.02f, 0.1f};

  if (pty_spawn(NULL, &master_fd, &child_pid) < 0) {
    perror("pty_spawn failed");
    return -1;
  }

  // Tell the shell how big the terminal is (rows x cols)
  pty_set_winsize(master_fd, grid_y_size, grid_x_size);

  // main loop
  while (!glfwWindowShouldClose(window)) {

    uint8_t buf[4096];
    ssize_t n = pty_read(master_fd, buf, sizeof(buf));

    if (n > 0) {
      for (ssize_t i = 0; i < n; i++) {
        uint8_t byte = buf[i];

        // Handle control characters
        switch (byte) {
        case '\r': // carriage return
          term_cursor_col = 0;
          break;
        case '\n': // newline
          term_cursor_col = 0;
          term_cursor_row++;
          if (term_cursor_row >= grid_y_size) {
            // scroll up one line
            memmove(grid, grid + grid_x_size,
                    sizeof(int) * grid_x_size * (grid_y_size - 1));
            memset(grid + grid_x_size * (grid_y_size - 1), ' ',
                   sizeof(int) * grid_x_size);
            term_cursor_row = grid_y_size - 1;
          }
          break;
        case '\b': // backspace
          if (term_cursor_col > 0)
            term_cursor_col--;
          grid[term_cursor_row * grid_x_size + term_cursor_col] = ' ';
          break;
        default:
          if (byte >= 32 && byte <= 126) {
            // printable ASCII
            grid[term_cursor_row * grid_x_size + term_cursor_col] = byte;
            term_cursor_col++;
            if (term_cursor_col >= grid_x_size) {
              term_cursor_col = 0;
              term_cursor_row++;
              if (term_cursor_row >= grid_y_size) {
                memmove(grid, grid + grid_x_size,
                        sizeof(int) * grid_x_size * (grid_y_size - 1));
                memset(grid + grid_x_size * (grid_y_size - 1), ' ',
                       sizeof(int) * grid_x_size);
                term_cursor_row = grid_y_size - 1;
              }
            }
          }
          break;
        }
      }
    }

    glClearColor(0.02f, 0.02f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // blink the cursor every 500 ms
    double now = glfwGetTime();
    if (now - last_toggle >= 0.5) {
      cursor_visible = !cursor_visible;
      last_toggle = now;
    }

    for (int y = 0; y < grid_y_size; y++) {
      int draw_y = grid_y_size - 1 - y; // flip vertically

      for (int x = 0; x < grid_x_size; x++) {
        char c = (char)grid[y * grid_x_size + x];
        float xpos = margin_x + x * x_spacing;
        float ypos = margin_y * 1.25 + draw_y * y_spacing;

        if (cursor_visible && y == term_cursor_row && x == term_cursor_col)
          RenderCursor(shaderProgram, text_scale, draw_y, x, fg, bg, c);
        else
          RenderChar(shaderProgram, c, xpos, ypos, text_scale, fg);
      }
    }

    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  // clean up
  free(grid);
  glfwTerminate();
  return 0;
}
