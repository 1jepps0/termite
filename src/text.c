#include <glad/glad.h>
#include <cglm/cglm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <shader.h>
#include <stdlib.h>
#include <string.h>

struct Character {
	unsigned int TextureID;  // ID handle of the glyph texture
	ivec2 Size;       // Size of glyph
	ivec2 Bearing;    // Offset from baseline to left/top of glyph
	unsigned int Advance;    // Offset to advance to next glyph
};

struct Character Characters[128];

unsigned int VBO;
unsigned int VAO;

int glyph_width;
int glyph_height;

int ascent;

int x_resolution = 1280;
int y_resolution = 720;

float x_spacing;
float y_spacing;
float margin_x = 10.0f;
float margin_y = 10.0f;

int grid_x_size;
int grid_y_size;

static const int base_x_resolution = 1280;
static const int base_y_resolution = 720;
static const float base_margin_x = 10.0f;
static const float base_margin_y = 10.0f;
static float base_text_scale = 0.35f;

void TextSetBaseScale(float scale) {
	base_text_scale = scale;
}

int *SetupGrid(void) {
	// setup a 2d array that stores all the possible positions of characters and their values
	// grid needs to be freed
	// grid[row * cols + col]

	grid_x_size = (int)((x_resolution - 2.0f * margin_x) / x_spacing);
	if (grid_x_size < 1)
		grid_x_size = 1;
	grid_y_size = (int)((y_resolution - margin_y) / y_spacing);
	if (grid_y_size < 1)
		grid_y_size = 1;

	int *grid = malloc((size_t)grid_x_size * (size_t)grid_y_size * sizeof(int));
	if (!grid)
		return NULL;

	// fill grid with spaces
	for (int y = 0; y < grid_y_size; y++) {
		for (int x = 0; x < grid_x_size; x++) {
			grid[y * grid_x_size + x] = ' ';
		}
	}

	return grid;
}


int SetupCharacters(void) {
  
	initialize_VBO_VAO(&VBO, &VAO);
	
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		printf("ERROR::FREETYPE: Could not init FreeType Library\n");
		return -1;
	}

	FT_Face face;
	if (FT_New_Face(ft, "../fonts/JetBrainsMono-Bold.ttf", 0, &face))
	{
		printf("ERROR::FREETYPE: Failed to load font\n"); 
		return -1;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);
	if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
	{
		printf("ERROR::FREETYTPE: Failed to load Glyph");
		return -1;
	}


	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
  
	for (unsigned char c = 0; c < 128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			printf("ERROR::FREETYTPE: Failed to load Glyph\n");
			continue;
		}

		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);

		glyph_width  = face->glyph->advance.x >> 6; // divide by 64
		glyph_height = face->size->metrics.height >> 6; // ascent+descent
		ascent = face->size->metrics.ascender >> 6;


		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// now store character for later use
		struct Character character = {
			texture,
			{ face->glyph->bitmap.width, face->glyph->bitmap.rows },   // size
			{ face->glyph->bitmap_left, face->glyph->bitmap_top },     // bearing
			(unsigned int)(face->glyph->advance.x)                // advance 
		};

		Characters[(unsigned char)c] = character;
	}

	// free resources once glyph processing is done
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	
	return 0;
}

void RenderChar(GLuint shaderProgram, char character, float x, float y, float scale, vec3 color)
{
  // activate corresponding render state	
	glUseProgram(shaderProgram);
	glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color[0], color[1], color[2]);
	glUniform1i(glGetUniformLocation(shaderProgram, "solid"), 0); 
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);

	struct Character ch = Characters[(unsigned char)character];

	float xpos = x + ch.Bearing[0] * scale;
	float ypos = y - (ch.Size[1] - ch.Bearing[1]) * scale;

	float w = ch.Size[0] * scale;
	float h = ch.Size[1] * scale;
	// update VBO for each character
	float vertices[6][4] = {
		{ xpos,     ypos + h,   0.0f, 0.0f },            
		{ xpos,     ypos,       0.0f, 1.0f },
		{ xpos + w, ypos,       1.0f, 1.0f },

		{ xpos,     ypos + h,   0.0f, 0.0f },
		{ xpos + w, ypos,       1.0f, 1.0f },
		{ xpos + w, ypos + h,   1.0f, 0.0f }           
	};
	// render glyph texture over quad
	glBindTexture(GL_TEXTURE_2D, ch.TextureID);
	// update content of VBO memory
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// render quad
	glDrawArrays(GL_TRIANGLES, 0, 6);
	// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
	x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void RenderCursor(GLuint shaderProgram, float text_scale,  int row, int col, vec3 fg, vec3 bg, char c) {
    float x = margin_x + col * x_spacing;
    float baseline = margin_y * 1.25f + row * y_spacing;

    struct Character reference = Characters[(unsigned char)'X'];
    float extra = (y_spacing - reference.Size[1] * text_scale) * 0.5f;
    float y_bottom = baseline - (reference.Size[1] - reference.Bearing[1]) * text_scale - extra;
    float y_top = y_bottom + y_spacing;
    float w = x_spacing;

    vec3 cursor_fg = { bg[0], bg[1], bg[2] };
    vec3 cursor_bg = { fg[0], fg[1], fg[2] };

    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), cursor_bg[0], cursor_bg[1], cursor_bg[2]);
    glUniform1i(glGetUniformLocation(shaderProgram, "solid"), 1);

    float vertices[6][4] = {
        { x,     y_bottom,   0.0f, 0.0f },
        { x,     y_top,      0.0f, 1.0f },
        { x+w,   y_top,      1.0f, 1.0f },

        { x,     y_bottom,   0.0f, 0.0f },
        { x+w,   y_top,      1.0f, 1.0f },
        { x+w,   y_bottom,   1.0f, 0.0f }
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUniform1i(glGetUniformLocation(shaderProgram, "solid"), 0);
    RenderChar(shaderProgram, c, x, baseline, text_scale, cursor_fg);
}

int *ResizeGrid(int *grid, int new_width, int new_height, float *text_scale, int *cursor_row, int *cursor_col, int *scroll_top, int *scroll_bottom) {
    if (new_width <= 0 || new_height <= 0 || glyph_width == 0 || glyph_height == 0)
        return grid;

    float width_ratio = (float)new_width / (float)base_x_resolution;
    float height_ratio = (float)new_height / (float)base_y_resolution;

    float new_scale = base_text_scale * height_ratio;
    if (new_scale < 0.01f)
        new_scale = 0.01f;

    x_resolution = new_width;
    y_resolution = new_height;

    margin_x = base_margin_x * width_ratio;
    margin_y = base_margin_y * height_ratio;

    x_spacing = glyph_width * new_scale;
    y_spacing = glyph_height * new_scale;

    if (x_spacing < 1.0f)
        x_spacing = 1.0f;
    if (y_spacing < 1.0f)
        y_spacing = 1.0f;

    int old_cols = grid_x_size;
    int old_rows = grid_y_size;

    grid_x_size = (int)((x_resolution - 2.0f * margin_x) / x_spacing);
    if (grid_x_size < 1)
        grid_x_size = 1;
    grid_y_size = (int)((y_resolution - margin_y) / y_spacing);
    if (grid_y_size < 1)
        grid_y_size = 1;

    size_t total_cells = (size_t)grid_x_size * (size_t)grid_y_size;
    int *new_grid = malloc(total_cells * sizeof(int));
    if (!new_grid)
        return grid;

    for (size_t idx = 0; idx < total_cells; idx++)
        new_grid[idx] = ' ';

    int copy_rows = old_rows < grid_y_size ? old_rows : grid_y_size;
    int copy_cols = old_cols < grid_x_size ? old_cols : grid_x_size;

    for (int y = 0; y < copy_rows; y++) {
        memcpy(new_grid + y * grid_x_size,
               grid + y * old_cols,
               (size_t)copy_cols * sizeof(int));
    }

    free(grid);

    if (*cursor_row >= grid_y_size)
        *cursor_row = grid_y_size - 1;
    if (*cursor_row < 0)
        *cursor_row = 0;
    if (*cursor_col >= grid_x_size)
        *cursor_col = grid_x_size - 1;
    if (*cursor_col < 0)
        *cursor_col = 0;

    if (*scroll_top < 0)
        *scroll_top = 0;
    if (*scroll_top >= grid_y_size)
        *scroll_top = grid_y_size - 1;
    if (*scroll_bottom < *scroll_top)
        *scroll_bottom = *scroll_top;
    if (*scroll_bottom >= grid_y_size)
        *scroll_bottom = grid_y_size - 1;

    *text_scale = new_scale;
    return new_grid;
}

