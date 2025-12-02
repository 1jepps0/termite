#ifndef TEXT_H
#define TEXT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

// describe glyph metrics used for rendering
struct Character {
	unsigned int TextureID;  // glyph texture id
	ivec2 Size;       // glyph size in pixels
	ivec2 Bearing;    // offset from baseline to top left
	unsigned int Advance;    // advance to next glyph
};

extern int glyph_width;
extern int glyph_height;
extern int ascent;

extern struct Character Characters[128];

extern int x_resolution;
extern int y_resolution;

extern float x_spacing;
extern float y_spacing;
extern float margin_x;   // pixel margin on x axis
extern float margin_y;   // pixel margin on y axis

extern int grid_x_size;
extern int grid_y_size;

// allocate a fresh grid filled with spaces
int *text_setup_grid(void);
// set base scaling used when resizing
void text_set_base_scale(float scale);
// resize grid keeping existing characters
int *text_resize_grid(int *grid, int new_width, int new_height, float *text_scale, int *cursor_row, int *cursor_col, int *scroll_top, int *scroll_bottom);

// compile shader from source string
GLuint compile_shader(const char *source, GLenum type);
// create shader program from vertex and fragment sources
GLuint create_shader_program(const char *vertex_src, const char *fragment_src);
// load shader text from file
char *load_shader_source(const char *filepath);
// draw a single character quad
void text_render_char(GLuint shaderProgram, char character, float x, float y, float scale, vec3 color);
// upload glyph textures and metrics
int text_setup_characters(void);
// render cursor block with inverted colors
void text_render_cursor(GLuint shaderProgram, float text_scale, int row, int col, vec3 fg, vec3 bg, char c);

#endif

