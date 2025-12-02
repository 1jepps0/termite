#include <app.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include <pty_wrap.h>
#include <shader.h>
#include <terminal.h>
#include <text.h>
#include <window.h>

typedef struct AppState {
    int master_fd;
    int child_pid;
    GLuint shader_program;
    TerminalState terminal;
    vec3 fg_color;
    vec3 bg_color;
} AppState;

static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void char_callback(GLFWwindow *window, unsigned int codepoint);
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void app_cleanup(AppState *app, GLFWwindow *window);

int app_run(void) {
    // initialize application state
    AppState app = {
        .master_fd = -1,
        .child_pid = -1,
        .shader_program = 0,
        .terminal = {0},
        .fg_color = {0.9f, 0.9f, 1.0f},
        .bg_color = {0.02f, 0.02f, 0.1f},
    };

    // create window and rendering context
    GLFWwindow *window = window_initialize();
    if (!window) {
        return -1;
    }

    // route glfw callbacks through application state
    glfwSetWindowUserPointer(window, &app);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetKeyCallback(window, key_callback);

    // load freetype glyph atlas
    if (text_setup_characters() != 0) {
        app_cleanup(&app, window);
        return -1;
    }

    // prepare shader program for text rendering
    app.shader_program = initialize_shader();
    if (app.shader_program == 0) {
        app_cleanup(&app, window);
        return -1;
    }

    // allocate grid space and parser state
    const float initial_scale = 0.35f;
    if (terminal_init(&app.terminal, initial_scale) != 0) {
        app_cleanup(&app, window);
        return -1;
    }

    // launch terminal session with pty
    if (pty_spawn(NULL, &app.master_fd, &app.child_pid) < 0) {
        perror("pty_spawn failed");
        app_cleanup(&app, window);
        return -1;
    }

    // notify child process of visible grid size
    pty_set_winsize(app.master_fd, grid_y_size, grid_x_size);

    double initial_time = glfwGetTime();
    terminal_on_input_activity(&app.terminal, initial_time);

    // get input and draw frames until window closes
    while (!glfwWindowShouldClose(window)) {
        uint8_t buf[4096];
        ssize_t n = pty_read(app.master_fd, buf, sizeof(buf));
        if (n > 0) {
            double input_now = glfwGetTime();
            terminal_on_input_activity(&app.terminal, input_now);
            terminal_process_data(&app.terminal, buf, (size_t)n);
        }

        // clear framebuffer using background color
        glClearColor(app.bg_color[0], app.bg_color[1], app.bg_color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // handle cursor blink timing and render grid
        double now = glfwGetTime();
        terminal_update_cursor(&app.terminal, now);
        terminal_render(&app.terminal, app.shader_program, app.fg_color, app.bg_color);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    app_cleanup(&app, window);
    return 0;
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    AppState *app = glfwGetWindowUserPointer(window);
    if (!app || width <= 0 || height <= 0)
        return;

    // update viewport and projection to match new framebuffer size
    glViewport(0, 0, width, height);
    if (app->shader_program != 0)
        shader_update_projection(app->shader_program, width, height);

    if (terminal_resize(&app->terminal, width, height) && app->master_fd > 0) {
        // resize terminal to hosted shell
        pty_set_winsize(app->master_fd, grid_y_size, grid_x_size);
    }
}

static void char_callback(GLFWwindow *window, unsigned int codepoint) {
    AppState *app = glfwGetWindowUserPointer(window);
    if (!app || app->master_fd < 0)
        return;

    // forward printable characters to shell
    char c = (char)codepoint;
    pty_write(app->master_fd, &c, 1);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    AppState *app = glfwGetWindowUserPointer(window);
    if (!app || app->master_fd < 0)
        return;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ENTER:
            // send carriage return on enter
            pty_write(app->master_fd, "\r", 1);
            break;
        case GLFW_KEY_BACKSPACE:
            // send del for backspace key
            pty_write(app->master_fd, "\x7F", 1);
            break;
        case GLFW_KEY_UP:
            // forward arrow key escape sequences
            pty_write(app->master_fd, "\x1b[A", 3);
            break;
        case GLFW_KEY_DOWN:
            pty_write(app->master_fd, "\x1b[B", 3);
            break;
        case GLFW_KEY_RIGHT:
            pty_write(app->master_fd, "\x1b[C", 3);
            break;
        case GLFW_KEY_LEFT:
            pty_write(app->master_fd, "\x1b[D", 3);
            break;
        case GLFW_KEY_C:
            if (mods & GLFW_MOD_CONTROL)
                // send interrupt signal on ctrl c
                pty_write(app->master_fd, "\x03", 1);
            break;
        default:
            break;
        }
    }
}

static void app_cleanup(AppState *app, GLFWwindow *window) {
    // close pty master descriptor
    if (app->master_fd >= 0) {
        close(app->master_fd);
        app->master_fd = -1;
    }

    // release terminal resources
    terminal_free(&app->terminal);

    // destroy glfw window before terminating
    if (window)
        glfwDestroyWindow(window);

    glfwTerminate();
}

