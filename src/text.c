#include <glad/glad.h>
#include <cglm/cglm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <shader.h>

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

int x_resolution =  1280;
int y_resolution = 720;

int x_spacing;
int y_spacing;
int margin_x = 10;   // pixels on left/right
int margin_y = 10;   // pixels on top/bottom

int grid_x_size;
int grid_y_size;

int cursor_row = 0;
int cursor_col = 0;




int* SetupGrid() {
	// setup a 2d array that stores all the possible positions of characters and their values
	// grid needs to be freed
	// grid[row * cols + col]

	grid_x_size = (x_resolution - 2 * margin_x) / x_spacing;
	grid_y_size = (y_resolution - 1 * margin_y) / y_spacing;


	int *grid = malloc(grid_x_size * grid_y_size * sizeof(int));


	// fill grid with spaces
	for (int y = 0; y < grid_y_size; y++) {
		for (int x = 0; x < grid_x_size; x++) {
			grid[y * grid_x_size + x] = ' ';
		}
	}

	return grid;
}



int SetupCharacters() {


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
    float y = margin_y * 1.25f + row * y_spacing; // baseline position
	
    float w = x_spacing;
    float h = y_spacing;

    // swap fg/bg for inverted look
    vec3 cursor_fg = { bg[0], bg[1], bg[2] };
    vec3 cursor_bg = { fg[0], fg[1], fg[2] };

    // draw background quad (cell-sized, behind text)
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"),
                cursor_bg[0], cursor_bg[1], cursor_bg[2]);
    glUniform1i(glGetUniformLocation(shaderProgram, "solid"), 1);

    float vertices[6][4] = {
        { x,     y,     0.0f, 0.0f },
        { x,     y+h,   0.0f, 1.0f },
        { x+w,   y+h,   1.0f, 1.0f },

        { x,     y,     0.0f, 0.0f },
        { x+w,   y+h,   1.0f, 1.0f },
        { x+w,   y,     1.0f, 0.0f }
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // draw character on top, inverted
    glUniform1i(glGetUniformLocation(shaderProgram, "solid"), 0);
    RenderChar(shaderProgram, c, x, y, text_scale, cursor_fg);
}

