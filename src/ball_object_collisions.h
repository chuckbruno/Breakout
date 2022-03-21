#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "game_object.h"
#include "texture.h"

class BallObject : public GameObject
{
public:
	// ball state
	float Radius;
	bool Stuck;
	// constructor(s)
	BallObject();
	BallObject(glm::vec2 pos, float radius, glm::vec2 velocity, Texture2D sprite);
	// moves the ball, keeping it constrained within the window bounds (except edge); return new position
	glm::vec2 Move(float dt, unsigned int window_width);
	// resets the ball to original state with given position and velocity
	void Reset(glm::vec2 position, glm::vec2 velocity);
};