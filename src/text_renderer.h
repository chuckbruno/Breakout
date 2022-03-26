#pragma once


#include <map>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "shader.h"


// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	unsigned int TextureID; // ID handle of the glyph texture;
	glm::vec2 Size;   // size of glyph
	glm::vec2 Bearing; // offset from baseline to left/top of glyph
	unsigned int Advance;  // horizontal offset to advance to next glyph
};


class TextRenderer
{
public:
	// holds a list of pre-compiled Characters
	std::map<char, Character> Characters;
	// shader used for text rendering
	Shader TextShader;
	// constructor
	TextRenderer(unsigned int width, unsigned int height);
	// pre-compiles a list of characters from the given font
	void Load(std::string font, unsigned int fontSize);
	// renders a string of text using the precompiled list of characters
	void RenderText(std::string text, float x, float y, float scale, glm::vec3 color = glm::vec3(1.0f));
private:
	// render state
	unsigned int VAO, VBO;
};