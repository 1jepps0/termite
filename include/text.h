#ifndef TEXT_H
#define TEXT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

struct Character {
	unsigned int TextureID;  // ID handle of the glyph texture
	ivec2 Size;       // Size of glyph
	ivec2 Bearing;    // Offset from baseline to left/top of glyph
	unsigned int Advance;    // Offset to advance to next glyph
};

extern int glyph_width;
extern int glyph_height;
extern int ascent;

extern struct Character Characters[128];

extern int x_resolution;
extern int y_resolution;

extern float x_spacing;
extern float y_spacing;
extern float margin_x;   // pixels on left/right
extern float margin_y;   // pixels on top/bottom

extern int grid_x_size;
extern int grid_y_size;

int *SetupGrid(void);
void TextSetBaseScale(float scale);
int *ResizeGrid(int *grid, int new_width, int new_height, float *text_scale, int *cursor_row, int *cursor_col, int *scroll_top, int *scroll_bottom);

GLuint compile_shader(const char *source, GLenum type);
GLuint create_shader_program(const char *vertex_src, const char *fragment_src);
char *load_shader_source(const char *filepath);
void RenderChar(GLuint shaderProgram, char character, float x, float y, float scale, vec3 color);
int SetupCharacters(void);
void RenderCursor(GLuint shaderProgram, float text_scale, int row, int col, vec3 fg, vec3 bg, char c);

#endif

