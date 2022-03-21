#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "game_object.h"
#include "sprite_renderer.h"
#include "resource_manager.h"


class GameLevel
{
public:
	// level state
	std::vector<GameObject> Bricks;
	// constructor
	GameLevel(){}
	// loads level from file
	void Load(const char* file, unsigned int levelWidth, unsigned int levelHeight);
	// render level
	void Draw(SpriteRenderer& renderer);
	// check if the level is completed (all non-solid titles are destroyed)
	bool IsCompleted();
private:
	void Init(std::vector<std::vector<unsigned int>> tileData, unsigned int levelWidth, unsigned int levelHeight);
};