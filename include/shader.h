#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <cglm/cglm.h>

extern mat4 projection;

char *load_shader_source(const char *filepath);
GLuint compile_shader(const char *source, GLenum type);
GLuint create_shader_program(const char *vertex_src, const char *fragment_src);
GLuint initialize_shader(void);
void initialize_VBO_VAO(unsigned int *VBO, unsigned int *VAO);
void shader_update_projection(GLuint shaderProgram, int width, int height);

#endif // SHADER_H

