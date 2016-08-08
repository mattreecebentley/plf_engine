#include <SDL2/SDL.h>

#include "plf_window.h"
#include "plf_utility.h"
#include "plf_renderer.h"


namespace plf
{


renderer::renderer(SDL_Window *window, const int logical_width, const int logical_height, const VSYNC_MODE vsync_mode):
	s_renderer(NULL),
	width(logical_width),
	height(logical_height)
{
	Uint32 vsync_flag = (vsync_mode == VSYNC_ON) ? SDL_RENDERER_PRESENTVSYNC : 0;

	// Try for hardware acceleration, vsync, rendering to textures, fallback to software rendering without vsync if unavailable:
	s_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|vsync_flag|SDL_RENDERER_TARGETTEXTURE);

	if (s_renderer == NULL)
	{
		std::clog << "plf::renderer constructor: Renderer could not be created, trying without texture rendering. SDL_Error:" << SDL_GetError() << std::endl;

		s_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|vsync_flag);

		if (s_renderer == NULL)
		{
			std::clog << "plf::renderer constructor: Renderer could not be created, trying without VSYNC. SDL_Error:" << SDL_GetError() << std::endl;
		
			s_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

			if (s_renderer == NULL)
			{
				std::clog << "plf::renderer constructor: Renderer could not be created, trying software with VSYNC. SDL_Error:" << SDL_GetError() << std::endl;

				s_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE|vsync_flag);

				if (s_renderer == NULL)
				{
					std::clog << "plf::renderer constructor: Renderer could not be created, trying software without VSYNC. SDL_Error:" << SDL_GetError() << std::endl;
				
					s_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

					plf_fail_if (s_renderer == NULL, "plf::renderer constructor: Renderer could not be created! SDL_Error:" << SDL_GetError());
				}
			}
		}
	}
	
	SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", "linear");
	
	if (width <= 0 || height <= 0) // If this is so, set to window's settings.
	{
		SDL_GetWindowSize(window, &width, &height);
	}
	
	SDL_RenderSetLogicalSize(s_renderer, width, height);
	SDL_RenderClear(s_renderer);
	
	SDL_RendererInfo s_renderer_info;
	SDL_GetRendererInfo(s_renderer, &s_renderer_info);

	std::clog << "plf::renderer created, using " << s_renderer_info.name << ", max texture width/height = " << s_renderer_info.max_texture_width << "/" << s_renderer_info.max_texture_height << "." << std::endl;
	
	
	// *Determine pixelformats*:
	
	// Create surface and texture to find texture and surface pixelformats, and also to test surface and texture creation:
	SDL_Surface *test_surface = create_surface(1, 1);

	plf_fail_if (test_surface == NULL, "plf::renderer Constructor: Could not create RGB test surface! SDL_Error: " << SDL_GetError());
	
	std::clog << "plf::renderer: RGB test surface created successfully." << std::endl;
	SDL_FreeSurface(test_surface);
	

	// Code adapted from SDL_CreateTextureFromSurface:
	default_texture_pixel_format = s_renderer_info.texture_formats[0];
	const Uint32 number_of_renderer_formats = s_renderer_info.num_texture_formats;
	
	for (Uint32 current_format_number = 0; current_format_number < number_of_renderer_formats; ++current_format_number)
	{
		if (!SDL_ISPIXELFORMAT_FOURCC(s_renderer_info.texture_formats[current_format_number]) && SDL_ISPIXELFORMAT_ALPHA(s_renderer_info.texture_formats[current_format_number]) == SDL_TRUE)
		{
			default_texture_pixel_format = s_renderer_info.texture_formats[current_format_number];
			break;
		}
	}
	
	SDL_Texture *test_texture = SDL_CreateTexture(s_renderer, default_texture_pixel_format, SDL_TEXTUREACCESS_STATIC, 1, 1);

	plf_fail_if (test_texture == NULL, "plf::renderer Constructor: Could not create RGB test texture! SDL_Error: " << SDL_GetError());
	
	SDL_QueryTexture(test_texture, &default_surface_pixel_format, NULL, NULL, NULL);
	std::clog << "plf::renderer: RGB test texture created successfully. Pixel formats established." << std::endl;

	if (SDL_SetTextureAlphaMod(test_texture, 128) < 0)
	{
		std::clog << "plf::renderer Constructor possible issue: Texture alpha modulation not supported. SDL_Error: " << SDL_GetError() << "." << std::endl;
	}
	
	if (SDL_SetTextureColorMod(test_texture, 75, 25, 255) < 0)
	{
		std::clog << "plf::renderer Constructor possible issue: Texture color modulation not supported. SDL_Error: " << SDL_GetError() << "." << std::endl;
	}
	
	SDL_DestroyTexture(test_texture);
	// * end determining pixelformats *
}



renderer::~renderer()
{
	// Destroy renderer
	SDL_DestroyRenderer(s_renderer);
}



SDL_Renderer* renderer::get()
{
	return s_renderer;
}



SDL_RendererInfo renderer::get_info()
{
	SDL_RendererInfo s_renderer_info;
   SDL_GetRendererInfo(s_renderer, &s_renderer_info);
   return s_renderer_info;
}



void renderer::display_frame()
{
	SDL_RenderPresent(s_renderer);
}



void renderer::clear_renderer()
{
	SDL_RenderClear(s_renderer);
}



void renderer::clear_screen()
{
	clear_renderer();
	display_frame();
}



void renderer::get_dimensions(int &return_width, int &return_height)
{
	return_width = width;
	return_height = height;
}



}