#include "game.h"
#include "ball_object_collisions.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "particle_generator.h"
#include "post_processor.h"

#include <iostream>

// Game-related State data
SpriteRenderer* Renderer;
GameObject* Player;
BallObject* Ball;
ParticleGenerator* Particles;
PostProcessor* Effects;

float ShakeTime = 0.0f;


Game::Game(unsigned int width, unsigned int height)
	: State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{

}

Game::~Game()
{
	delete Renderer;
	delete Player;
	delete Ball;
	delete Particles;
	delete Effects;
}

void Game::Init()
{
	// load shaders
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
	ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", nullptr, "particle");
	ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.frag", nullptr, "postprocessing");
	// configure shaders
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
		static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
	ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
	ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
	// load texture
	ResourceManager::LoadTexture("textures/background.jpg", false, "background");
	ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
	ResourceManager::LoadTexture("textures/block.png", false, "block");
	ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
	ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
	ResourceManager::LoadTexture("textures/particle.png", true, "particle");
	ResourceManager::LoadTexture("textures/powerup_speed.png", true, "powerup_speed");
	ResourceManager::LoadTexture("textures/powerup_sticky.png", true, "powerup_sticky");
	ResourceManager::LoadTexture("textures/powerup_increase.png", true, "powerup_increase");
	ResourceManager::LoadTexture("textures/powerup_confuse.png", true, "powerup_confuse");
	ResourceManager::LoadTexture("textures/powerup_chaos.png", true, "powerup_chaos");
	ResourceManager::LoadTexture("textures/powerup_passthrough.png", true, "powerup_passthrough");

	// set render-specific controls
	auto _shader = ResourceManager::GetShader("sprite");
	Renderer = new SpriteRenderer(_shader);
	Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
	Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
	// load levels
	GameLevel one;
	one.Load("levels/one.lvl", this->Width, this->Height / 2);
	GameLevel two;
	two.Load("levels/two.lvl", this->Width, this->Height / 2);
	GameLevel three;
	three.Load("levels/three.lvl", this->Width, this->Height / 2);
	GameLevel four;
	four.Load("levels/four.lvl", this->Width, this->Height / 2);
	this->Levels.push_back(one);
	this->Levels.push_back(two);
	this->Levels.push_back(three);
	this->Levels.push_back(four);
	this->Level = 0;

	glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
	Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
	glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
	Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
}

void Game::Update(float dt)
{
	// update objects
	Ball->Move(dt, this->Width);
	// check for collisions
	this->DoCollisions();
	// update particles
	Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));
	// reduce shake time
	if (ShakeTime > 0.0f)
	{
		ShakeTime -= dt;
		if (ShakeTime <= 0.0f)
			Effects->Shake = false;
	}
	// check loss condition
	if (Ball->Position.y >= this->Height) // did ball reach bottom edge?
	{
		this->ResetLevel();
		this->ResetPlayer();
	}
}

void Game::ProcessInput(float dt)
{
	if (this->State == GAME_ACTIVE)
	{
		float velocity = PLAYER_VELOCITY * dt;
		// move playerboard
		if (this->Keys[GLFW_KEY_A])
		{
			if (Player->Position.x >= 0.0f)
			{
				Player->Position.x -= velocity;
				if (Ball->Stuck)
					Ball->Position.x -= velocity;
			}
		}
		if (this->Keys[GLFW_KEY_D])
		{
			if (Player->Position.x <= this->Width - Player->Size.x)
			{
				Player->Position.x += velocity;
				if (Ball->Stuck)
					Ball->Position.x += velocity;
			}
		}

		if (this->Keys[GLFW_KEY_SPACE])
		{
			Ball->Stuck = false;
		}
	}
}

void Game::Render()
{
	if (this->State == GAME_ACTIVE)
	{
		// begin rendering to postprocessing framebuffer
		Effects->BeginRender();
		// draw background
		auto _background = ResourceManager::GetTexture("background");
		Renderer->DrawSprite(_background, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
		// draw level
		this->Levels[this->Level].Draw(*Renderer);
		// draw player
		Player->Draw(*Renderer);
		// draw PowerUps
		for (PowerUp& powerUp : this->PowerUps)
			if (!powerUp.Destroyed)
				powerUp.Draw(*Renderer);
		// draw particles
		Particles->Draw();
		// draw ball
		Ball->Draw(*Renderer);
		// end rendering to postprocessing framebuffer
		Effects->EndRender();
		// render postprocessing quad
		Effects->Render(glfwGetTime());
	
	}
	//auto _texture = ResourceManager::GetTexture("face");
	//Renderer->DrawSprite(_texture, glm::vec2(200.0f, 200.0f), glm::vec2(300.0f, 400.0f), 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Game::ResetLevel()
{
	if (this->Level == 0)
		this->Levels[0].Load("levels/one.lvl", this->Width, this->Height / 2);
	else if(this->Level == 1)
		this->Levels[1].Load("levels/two.lvl", this->Width, this->Height / 2);
	else if(this->Level == 2)
		this->Levels[2].Load("levels/three.lvl", this->Width, this->Height / 2);
	else if(this->Level == 3)
		this->Levels[3].Load("levels/four.lvl", this->Width, this->Height / 2);

}

void Game::ResetPlayer()
{
	// reset player/ball stats
	Player->Size = PLAYER_SIZE;
	Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
	Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);
}

// powerups
bool IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type);

void Game::UpdatePowerUps(float dt)
{
	for (PowerUp& powerUp : this->PowerUps)
	{
		powerUp.Position += powerUp.Velocity * dt;
		if (powerUp.Actived)
		{
			powerUp.Duration -= dt;
			if (powerUp.Duration < 0.0f)
			{
				// remove powerup from list (will later be removed)
				powerUp.Actived = false;
				if (powerUp.Type = "sticky")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
					{
						//only reset if no other PowerUp of type sticky is active
						Ball->Stuck = false;
						Player->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "pass-through")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
					{
						// only reset if no other PowerUp of type pass-through is active
						Ball->PassThrough = false;
						Ball->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "confuse")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
					{
						// only reset if no other PowerUp of type confuse is active
						Effects->Confuse = false;
					}
				}
				else if (powerUp.Type == "chaos")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
					{
						// only reset if no other PowerUp of type chaos is active
						Effects->Chaos = false;
					}
				}
			}
		}
	}

	// Remove all PowerUps from vector that are destroyed AND! actived (thus either off the map or finished)
	// Note we use a lambda expression to remove each PowerUp which is destroyed and not actived
	this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
		[](const PowerUp& powerUp) {return powerUp.Destroyed && !powerUp.Actived; }),
		this->PowerUps.end());
}

// collision detection
bool CheckCollision(GameObject& one, GameObject& two);
Collision CheckCollision(BallObject& one, GameObject& two);
Direction VectorDirection(glm::vec2 closet);

void Game::DoCollisions()
{
	for (GameObject& box : this->Levels[this->Level].Bricks)
	{
		if (!box.Destroyed)
		{
			Collision collision = CheckCollision(*Ball, box);
			if (std::get<0>(collision)) // if collision is true
			{
				// destroy block if not solid
				if (!box.IsSolid)
					box.Destroyed = true;
				else
				{
					// if block is solid, enable shake effect
					ShakeTime = 0.05;
					Effects->Shake = true;
				}
				// collision resolution
				Direction dir = std::get<1>(collision);
				glm::vec2 diff_vector = std::get<2>(collision);
				if (dir == LEFT || dir == RIGHT) // horizontal collision
				{
					Ball->Velocity.x = -Ball->Velocity.x; // reverse horizontal velocity
					// relocate
					float penetration = Ball->Radius - std::abs(diff_vector.x);
					if (dir == LEFT)
						Ball->Position.x += penetration; // move ball to right
					else
						Ball->Position.x -= penetration; // move ball to left
				}
				else // vertical collision
				{
					Ball->Velocity.y = -Ball->Velocity.y; // reverse vertical velocity
					// relocate
					float penetration = Ball->Radius - std::abs(diff_vector.y);
					if (dir == UP)
						Ball->Position.y -= penetration; // move ball back up
					else
						Ball->Position.y += penetration; // move ball back down
				}

			}
		}
	}
	
	// check collisions for player pad (unless stuck)
	Collision result = CheckCollision(*Ball, *Player);
	if (!Ball->Stuck && std::get<0>(result))
	{
		// check where it hit the board, and change velocity based on where it hit the board
		float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
		float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
		float percentage = distance / (Player->Size.x / 2.0f);

		// then move accordingly
		float strength = 2.0f;
		glm::vec2 oldVelocity = Ball->Velocity;
		Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		//Ball->Velocity.y = -Ball->Velocity.y;
		Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity); // keep speed consitent over both axes (multiply by length of old velocity)
		// fix sticky paddle
		Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
	}
}

bool CheckCollision(GameObject& one, GameObject& two)
{
	// collision x-axis?
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
		two.Position.x + two.Size.x >= one.Position.x;
	// collision y-axis?
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
		two.Position.y + two.Size.y >= one.Position.y;

	// collision only if on both axes
	return collisionX && collisionY;
}

Collision CheckCollision(BallObject& one, GameObject& two)
{
	// get center point circle first
	glm::vec2 center(one.Position + one.Radius);
	// calculate AABB info (center, half-extent)
	glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
	glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
	// get difference vector between both centers
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	// now that we know the clamped values, add this to AABB_center and we get the value of box closest to circle
	glm::vec2 closest = aabb_center + clamped;
	// now retrieve vector between center circle and closest point AABB and check if length < radius
	difference = closest - center;

	if (glm::length(difference) < one.Radius)
		return std::make_tuple(true, VectorDirection(difference), difference);
	else
		return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

Direction VectorDirection(glm::vec2 target)
{
	glm::vec2 compass[] = {
		glm::vec2(0.0f, 1.0f), // up
		glm::vec2(1.0f, 0.0f), // right
		glm::vec2(0.0f, -1.0f), // down
		glm::vec2(-1.0f, 0.0f) //left
	};

	float max = 0.0f;
	unsigned int best_match = -1;
	for (unsigned int i = 0; i < 4; ++i)
	{
		float dot_product = glm::dot(glm::normalize(target), compass[i]);
		if (dot_product > max)
		{
			max = dot_product;
			best_match = i;
		}
	}

	return (Direction)best_match;
}