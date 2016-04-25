#include <map>
#include <cassert>
#include <ctime>
#include <cstdio>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>


#include "plf_utility.h"
#include "plf_window.h"
#include "plf_renderer.h"
#include "plf_sound.h"
#include "plf_music.h"
#include "plf_atlas.h"
#include "plf_texture.h"
#include "plf_sprite.h"
#include "plf_entity.h"
#include "plf_layer.h"
#include "plf_engine.h"



namespace plf
{

engine::engine():
	window(NULL),
	renderer(NULL),
	atlas_manager(NULL),
	texture_manager(NULL),
	layers(NULL),
	entities(NULL),
	sprites(NULL),
	sound(NULL),
	music(NULL)
{
	std::clog << "plf::engine created. Date/time " << get_timedate_string() << ":" << std::endl;

	// Initialise SDL graphics and subsystems:
	plf_fail_if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER) < 0, "plf::engine constructor: SDL graphics and subsystems could not initialize! SDL_Error: " << SDL_GetError());

	std::clog << "SDL video, timer and events initialised." << std::endl;

	// Initialise SDL_IMG for use with PNGs only:
	plf_fail_if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0, "plf::engine constructor: SDL_img PNG or JPEG support could not initialize! Quitting.");

	std::clog << "SDL_img initialised with PNG and JPEG support." << std::endl;



	// Initialise sound and music:
	plf_fail_if (SDL_Init(SDL_INIT_AUDIO) < 0, "plf::engine constructor: SDL audio could not initialize! SDL_Error: " << SDL_GetError());
	std::clog << "SDL audio initialised." << std::endl;


	// Initialise SDM_Mixer for OGG and FLAC only:
	const int support_flags = MIX_INIT_OGG|MIX_INIT_FLAC;

	plf_fail_if ((Mix_Init(support_flags) & support_flags) != support_flags, "plf::engine constructor: SDL_mixer OGG or FLAC support could not initialize! Quitting.");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0)
	{
		std::clog << "plf::engine constructor: SDL_mixer OpenAudio could not initialize at 44khz! Error: " << Mix_GetError() << std::endl;

		// Try 48khz instead:
		plf_fail_if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 1024) != 0, "plf::engine constructor: SDL_mixer OpenAudio could not initialize at 48khz. Error: " << Mix_GetError());
	}
}




void engine::initialize(const char *window_name, const unsigned int window_width, const unsigned int window_height, const unsigned int renderer_width, const unsigned int renderer_height, const WINDOW_MODE window_mode, const VSYNC_MODE vsync_mode)
{
	std::clog << "plf::engine initializing." << std::endl;

	// Initialise Window:
	window = new plf::window(window_name, window_width, window_height, window_mode);
	std::clog << "SDL Window '" << window_name << "' created with dimensions " << window_width << " * " << window_height << "." << std::endl;


	// Initialise Renderer:
	renderer = new plf::renderer(window->get(), renderer_width, renderer_height, vsync_mode);
	std::clog << "SDL Renderer created with logical dimensions " << renderer_width << " * " << renderer_height << "." << std::endl;


	atlas_manager = new plf::atlas_manager(renderer);
	texture_manager = new plf::texture_manager(renderer, atlas_manager);
	sprites = new plf::sprite_manager(texture_manager);


	// Initialise sound manager:
	int width, height;
	renderer->get_dimensions(width, height);
	sound = new plf::sound_manager(width, height);

	std::clog << "plf::sound_manager initialised with stereo center at " << width / 2 << ", " << height / 2 << "." << std::endl;

	// Initialise music manager:
	music = new plf::music_manager(sound);

	// Initialise entities:
	entities = new plf::entity_manager(sound);

	// Initialise layers:
	layers = new plf::layer_manager();

	// Seed C randomiser:
	srand (static_cast<unsigned int>(time(NULL)));
}



engine::~engine()
{
	delete sprites;
	delete entities;
	delete layers;
	delete music;
	delete sound;	
	delete texture_manager;
	delete atlas_manager;
	delete renderer;
	delete window;
	
	// Destroy SDL_Mixer:
	Mix_CloseAudio();
	Mix_Quit();
	
	// Destroy SDL_image:
	IMG_Quit();
	
	// Destroy SDL:
	SDL_Quit();
}



SDL_DisplayMode engine::get_current_display_mode()
{
	SDL_DisplayMode current_mode;
	plf_fail_if(SDL_GetCurrentDisplayMode(0, &current_mode) != 0, "plf::engine get_current_display_modes: could not get display mode! Quitting.");
	return current_mode;
}



void engine::get_all_display_modes(std::vector<SDL_DisplayMode> &display_modes)
{
	int num_display_modes = SDL_GetNumDisplayModes(0);
	
	plf_fail_if (num_display_modes == -1, "plf::engine get_all_display_modes: could not get display modes! Quitting.");

	display_modes.resize(num_display_modes);
	SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

	for (int current_mode = 0; current_mode != num_display_modes; ++current_mode)
	{
		SDL_GetDisplayMode(0, current_mode, &mode);
		display_modes[current_mode] = mode;
	}
	
}



int engine::set_scale_quality(const unsigned int quality_level)
{
	assert(quality_level <= 2); // must be between 0 and 2

	char quality[4];
	sprintf(quality, "%u", quality_level);
	
	if (SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", quality) == SDL_TRUE)
	{
		return 0;
	}
	
	return -1;
}

}