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

extern int x_spacing;
extern int y_spacing;
extern int margin_x;   // pixels on left/right
extern int margin_y;   // pixels on top/bottom

extern int grid_x_size;
extern int grid_y_size;

int* SetupGrid(); 

GLuint compile_shader(const char* source, GLenum type); 
GLuint create_shader_program(const char* vertex_src, const char* fragment_src);
char* load_shader_source(const char* filepath); 
void RenderChar(GLuint shaderProgram, char character, float x, float y, float scale, vec3 color);
int SetupCharacters();
void RenderCursor(GLuint shaderProgram, float text_scale, int row, int col, vec3 fg, vec3 bg, char c); 

#endif

