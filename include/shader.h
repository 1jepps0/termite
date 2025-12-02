#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <cglm/cglm.h>

// global orthographic projection matrix
extern mat4 projection;

// read shader source from a file path
char *load_shader_source(const char *filepath);
// compile individual shader stage from source
GLuint compile_shader(const char *source, GLenum type);
// link vertex and fragment shaders together
GLuint create_shader_program(const char *vertex_src, const char *fragment_src);
// create default shader program for text rendering
GLuint initialize_shader(void);
// set up quad rendering buffers
void initialize_VBO_VAO(unsigned int *VBO, unsigned int *VAO);
// refresh projection matrix uniforms when viewport changes
void shader_update_projection(GLuint shaderProgram, int width, int height);

#endif // SHADER_H

