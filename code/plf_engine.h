#ifndef PLF_ENGINE_H
#define PLF_ENGINE_H

#include <vector>

#include <SDL2/SDL.h>

#include "plf_log.h"
#include "plf_window.h"
#include "plf_renderer.h"
#include "plf_atlas.h"
#include "plf_texture.h"
#include "plf_sprite.h"
#include "plf_sound.h"
#include "plf_music.h"
#include "plf_entity.h"
#include "plf_layer.h"
#include "plf_math.h"



namespace plf
{

	class engine
	{
	public:
		plf::window *window;
		plf::renderer *renderer;
		plf::atlas_manager *atlas_manager;
		plf::texture_manager *texture_manager;
		plf::layer_manager *layers;
		plf::entity_manager *entities;
		plf::sprite_manager *sprites;
		plf::sound_manager *sound;
		plf::music_manager *music;
	
		engine();
		~engine();
		
		void initialize(const char *window_name, const unsigned int window_width, const unsigned int window_height, const unsigned int renderer_width, const unsigned int renderer_height, const WINDOW_MODE window_mode = WINDOWED, const VSYNC_MODE vsync_mode = VSYNC_OFF);
	
		SDL_DisplayMode get_current_display_mode();
		void get_all_display_modes(std::vector<SDL_DisplayMode> &display_modes);
	
		int set_scale_quality(const unsigned int quality_level); // Scaling algorithm for resized sprites: 0 = per-pixel, 1 = linear, 2 = anisotropic - linear is default
	};


}

#endif // PLF_ENGINE_H
