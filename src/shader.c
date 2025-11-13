#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <cglm/cglm.h>

mat4 projection;
static GLint projection_location = -1;

void initialize_VBO_VAO(unsigned int *VBO, unsigned int *VAO) {
	glGenVertexArrays(1, VAO);
	glGenBuffers(1, VBO);
	glBindVertexArray(*VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

char* load_shader_source(const char* filepath) {
    FILE* file = fopen(filepath, "rb");  // open in binary mode
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Allocate memory (+1 for null terminator)
    char* source = (char*)malloc(length + 1);
    if (!source) {
        fprintf(stderr, "Failed to allocate memory for shader\n");
        fclose(file);
        return NULL;
    }

    // Read file into memory
    fread(source, 1, length, file);
    source[length] = '\0';  // null terminate
    fclose(file);

    return source;
}


// compile a shader and check for errors
GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        fprintf(stderr, "Shader compile error: %s\n", info);
    }

    return shader;
}


// link vertex & fragment shaders into a program
GLuint create_shader_program(const char* vertex_src, const char* fragment_src) {
    GLuint vertexShader = compile_shader(vertex_src, GL_VERTEX_SHADER);
    GLuint fragmentShader = compile_shader(fragment_src, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, NULL, info);
        fprintf(stderr, "Program link error: %s\n", info);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}



GLuint initialize_shader() {

	// enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Load shader files
	char* vertexSource = load_shader_source("../src/vertex.glsl");
	char* fragmentSource = load_shader_source("../src/fragment.glsl");

	if (!vertexSource || !fragmentSource) {
		printf("ERROR: Shader source could not be loaded\n");
	}

	// Create shader program
	GLuint shaderProgram = create_shader_program(vertexSource, fragmentSource);

	// Free memory
	free(vertexSource);
	free(fragmentSource);
	
	// set projection matrix
	glm_ortho(0.0f, 1280.0f, 0.0f, 720.0f, -1.0f, 1.0f, projection);

	// Use the shader
	projection_location = glGetUniformLocation(shaderProgram, "projection");
	glUseProgram(shaderProgram);
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, (const GLfloat*)projection);
	glUniform1i(glGetUniformLocation(shaderProgram, "text"), 0); // GL_TEXTURE0

	return shaderProgram;
}

void shader_update_projection(GLuint shaderProgram, int width, int height) {
    glm_ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f, projection);
    glUseProgram(shaderProgram);
    if (projection_location == -1)
        projection_location = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, (const GLfloat *)projection);
}


