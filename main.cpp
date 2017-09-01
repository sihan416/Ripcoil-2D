#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <ctime>
#include <cmath>

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game0: Ripcoil 2D";
		glm::uvec2 size = glm::uvec2(640, 640);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
	);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	SDL_ShowCursor(SDL_DISABLE);

	//------------  game state ------------

	glm::vec2 mouse = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball = glm::vec2(0.85f, 0.0f);
	glm::vec2 ball_velocity = glm::vec2(0.0f, 0.0f);
	srand((unsigned int)time(NULL));
	glm::vec2 pad1 = glm::vec2((float)(rand() % 100 - 50)/200.0f, 1.0f);
	glm::vec2 pad2 = glm::vec2(-1.0f, (float)(rand() % 100 - 50) / 200.0f);
	glm::vec2 pad3 = glm::vec2((float)(rand() % 100 - 50) / 200.0f, -1.0f);
	int curBounces = 0;
	int expectedBounces = 1;
	int lives = 3;
	float padLength = 1.0f;

	//------------  game loop ------------

	auto previous_time = std::chrono::high_resolution_clock::now();
	bool should_quit = false; 
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEMOTION) {
				mouse.x = (evt.motion.x + 0.5f) / float(config.size.x) * 2.0f - 1.0f;
				mouse.y = (evt.motion.y + 0.5f) / float(config.size.y) *-2.0f + 1.0f;
			} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
				if (ball_velocity.x == 0.0f && ball_velocity.y == 0.0f &&
					mouse.y - padLength / 2.0f < 0.05f && mouse.y + padLength / 2.0f > -0.05f) {
					float y_velocity = 2.0f* (ball.y - mouse.y);
					ball_velocity = glm::vec2(-sqrt(1.0f - y_velocity * y_velocity), y_velocity);
				}
			} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			ball += elapsed * ball_velocity;
			if (ball.y + 0.05f >= 0.9f && ball.x - 0.05f <= pad1.x + padLength / 2.0 && ball.x + 0.05f >= pad1.x - padLength / 2.0) {
				ball_velocity.y = -std::abs(ball_velocity.y);
				curBounces++;
			} else if (ball.x - 0.05f <= -0.9f && ball.y - 0.05f <= pad2.y + padLength / 2.0 && ball.y + 0.05f >= pad2.y - padLength / 2.0) {
				ball_velocity.x = std::abs(ball_velocity.x);
				curBounces++;
			} else if (ball.y - 0.05f <= -0.9f && ball.x - 0.05f <= pad3.x + padLength / 2.0 && ball.x + 0.05f >= pad3.x - padLength / 2.0) {
				ball_velocity.y = std::abs(ball_velocity.y);
				curBounces++;
			} else if (ball.x + 0.05f >= 0.9f && ball.y - 0.05f <= mouse.y + padLength / 2.0 && ball.y + 0.05f >= mouse.y - padLength / 2.0) {
				if (ball_velocity.x != 0.0f || ball_velocity.y != 0.0f) {
					float y_velocity = 2.0f* (ball.y - mouse.y);
					ball_velocity = glm::vec2(-sqrt(1.0f - y_velocity * y_velocity), y_velocity);
				}
				if (curBounces >= expectedBounces) {
					ball_velocity = glm::vec2(0.0f, 0.0f);
					ball = glm::vec2(0.85f, 0.0f);
					expectedBounces++;
					curBounces = 0;
					pad1 = glm::vec2((float)(rand() % 100 - 50) / 200.0f, 1.0f);
					pad2 = glm::vec2(-1.0f, (float)(rand() % 100 - 50) / 200.0f);
					pad3 = glm::vec2((float)(rand() % 100 - 50) / 200.0f, -1.0f);
				}
			} else if (ball.x - 0.05f <= -1.0f || ball.x + 0.05f >= 1.0f || ball.y - 0.05f <= -1.0f || ball.y + 0.05f >= 1.0f) {
				ball_velocity = glm::vec2(0.0f, 0.0f);
				ball = glm::vec2(0.85f, 0.0f);
				lives--;
				curBounces = 0;
				if (lives == 0)
					should_quit = true;
			}
		}

		//draw output:
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);


		{ //draw game state:
			Draw draw;
			draw.add_rectangle(glm::vec2(0.9f, mouse.y - padLength/2.0f),
				glm::vec2(1.0f, mouse.y + padLength/2.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			draw.add_rectangle(glm::vec2(-1.0f, pad2.y - padLength / 2.0f),
				glm::vec2(-0.9f, pad2.y + padLength / 2.0f),
				glm::u8vec4(0x00, 0x00, 0xff, 0xff));
			draw.add_rectangle(glm::vec2(pad1.x - padLength / 2.0f, 0.9f),
				glm::vec2(pad1.x + padLength / 2.0f, 1.0f),
				glm::u8vec4(0x00, 0x00, 0xff, 0xff));
			draw.add_rectangle(glm::vec2(pad1.x - padLength / 2.0f, -1.0f),
				glm::vec2(pad1.x + padLength / 2.0f, -0.9f),
				glm::u8vec4(0x00, 0x00, 0xff, 0xff));
			draw.add_rectangle(ball + glm::vec2(-0.05f,-0.05f), ball + glm::vec2(0.05f, 0.05f), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
			draw.draw();
		}


		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;
}
