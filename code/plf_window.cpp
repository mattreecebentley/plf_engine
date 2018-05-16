#include <SDL2/SDL.h>

#include "plf_window.h"
#include "plf_utility.h"


namespace plf
{

	window::window(const char *window_title, const unsigned int window_width, const unsigned int window_height, const WINDOW_MODE window_mode):
		p_window(NULL)
	{
		// Create a window, fallback to non-opengl if opengl not available, fallback to windowed if fullscreen specified but not available:
		Uint32 default_window_flags = SDL_WINDOW_SHOWN|SDL_WINDOW_INPUT_FOCUS;

		if (window_width != 0 && window_height != 0 && window_mode != FULLSCREEN_DESKTOP)
		{
			if (window_mode == FULLSCREEN)
			{
				p_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, default_window_flags|SDL_WINDOW_FULLSCREEN);

				if (p_window == NULL)
				{
					std::clog << "plf::window constructor: Window could not be created in fullscreen, trying without fullscreen. " << std::endl;
					p_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, default_window_flags);
				}
			}
			else // no fullscreen
			{
				std::clog << "plf::window constructor: Non-fullscreen window attempt - note: non-fullscreen modes slower by factor of 10, generally." << std::endl;
				p_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, default_window_flags);

				if (p_window == NULL)
				{
					std::clog << "plf::window constructor: Non-fullscreen window could not be created, trying fullscreen. " << std::endl;
					p_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, default_window_flags|SDL_WINDOW_FULLSCREEN);
				}
			}
		}
		else // Assume fullscreen desktop mode (window height and width == 0).
		{
			std::clog << "plf::window constructor: Fullscreen-desktop window attempt - note: non-fullscreen modes (including fullscreen-desktop) are often slower by factor of 10." << std::endl;
			p_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, default_window_flags|SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_BORDERLESS);

			if (p_window == NULL)
			{
				std::clog << "plf::window constructor: Fullscreen desktop window could not be created, trying regular fullscreen." << std::endl;
				p_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, default_window_flags|SDL_WINDOW_FULLSCREEN);
			}
		}

		plf_fail_if (p_window == NULL, "plf::window constructor: Error! Window could not be created!");
	}


	window::~window()
	{
		SDL_DestroyWindow(p_window);
	}


	SDL_Window* window::get()
	{
		return p_window;
	}


	SDL_Surface* window::get_surface()
	{
		return SDL_GetWindowSurface(p_window);
	}

}