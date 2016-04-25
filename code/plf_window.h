#ifndef PLF_WINDOW_H
#define PLF_WINDOW_H

#include <SDL2/SDL.h>

namespace plf
{

enum WINDOW_MODE
{
	FULLSCREEN,
	WINDOWED,
	FULLSCREEN_DESKTOP
};


class window
{
private:
	SDL_Window* p_window;
public:
	// Creates a new SDL_Window based on supplied parameters:
	window(const char *window_title, const unsigned int window_width, const unsigned int window_height, const WINDOW_MODE window_mode);
	~window();
	
	// Get a direct pointer to the SDL_Window:
	SDL_Window* get();
	
	// The following is largely for advanced SDL users who wish to do some complicated ops, for some reason:
	// Get a pointer to the SDL_Surface underlying the SDL_Window:
	SDL_Surface* get_surface();
};

}

#endif // window