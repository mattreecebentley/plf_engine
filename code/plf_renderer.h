#ifndef PLF_RENDERER_H
#define PLF_RENDERER_H

#include <SDL2/SDL.h>

#include "plf_window.h"


namespace plf
{

	enum VSYNC_MODE
	{
		VSYNC_ON,
		VSYNC_OFF
	};
	
	
	
	class renderer
	{
	private:
		SDL_Renderer *s_renderer;
		int width, height; // Logical resolutions of the renderer, as opposed to the screen it is projected onto
		Uint32 default_texture_pixel_format, default_surface_pixel_format; // These two may be different to each other
	public:
		// Construct an SDL_Renderer using the parameters provided, figure out texture and surface pixel formats, test renderer properties:
		renderer(SDL_Window *window, int logical_width, int logical_height, const VSYNC_MODE vsync_mode);
		~renderer();
	
		SDL_Renderer* get(); // Get a raw pointer to the SDL_Renderer
		SDL_RendererInfo get_info(); // Return SDL-encoded data about the renderer
	
		void get_dimensions(int &return_width, int &return_height);
		inline Uint32 get_surface_pixel_format() { return default_surface_pixel_format; };
		inline Uint32 get_texture_pixel_format() { return default_texture_pixel_format; };
	
		void display_frame(); // Display renderer content at present point in time
		void clear_renderer(); // This clears the renderer but does not actually update the screen
		void clear_screen(); // Calls clear_renderer and updates screen
	};


}
#endif // renderer_H
