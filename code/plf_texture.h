#ifndef PLF_TEXTURE_H
#define PLF_TEXTURE_H

#include <SDL2/SDL.h>


#include "plf_atlas.h"


namespace plf
{

	struct rgb
	{
		Uint8 r;
		Uint8 g;
		Uint8 b;
	};
	
	
	class texture
	{
	private:
		SDL_Texture *atlas_texture; // Underlying SDL_Texture of the texture atlas this texture is located within - this is a local cache - could be removed and be replaced with a call to atlas->get_texture
		SDL_Rect *atlas_coordinates; // Location of texture within atlas
		plf::atlas_node *node; // The specific node within the atlas - stored in case this texture is deleted and the atlas needs to be updated
		plf::atlas *atlas; // Pointed to the texture atlas this texture is located within - also preseved in case of deletion
		SDL_Renderer *s_renderer; // Stored locally for faster renderer access
		int renderer_width, renderer_height; // Stored locally for offscreen texture draw culling, avoid calls to SDL_Renderer
	public:
		texture(): node(NULL) {}; // Prevents segfault with derived class multitexture
		texture(renderer *p_renderer, atlas_manager *atlas_manager, SDL_Surface *surface);
		virtual ~texture();
	
		// center, x & y are not required to be non-const in this draw but they are in the multitexture virtual derivative:
		virtual int draw(int x, int y, const double size = 1, const double angle = 0, SDL_Point *center = NULL, const SDL_RendererFlip flip = SDL_FLIP_NONE, const Uint8 transparency = 255, const rgb *colormod = NULL);
	};
	
	
	
	class multitexture : public texture
	{
	private:
		struct segment
		{
			SDL_Texture *atlas_texture;
			SDL_Rect *atlas_coordinates;
			atlas_node *node;
			plf::atlas *atlas;
			int segment_x, segment_y;
		};
	
		SDL_Renderer *s_renderer;
		segment *segments; // to be filled with dynamically-allocated array storing the entirety of the image.
		segment *end_segment; // to not have to recalculate the value for every loop. It's either that or store the number of segments as a uint
	
		int total_width, total_height; // Total size of the entire image, as opposed to it's segments
		int renderer_width, renderer_height;
	public:
		multitexture(renderer *p_renderer, atlas_manager *atlas_manager, SDL_Surface *surface, const unsigned int maximum_width, const unsigned int maximum_height);
		~multitexture();
	
	    int draw(int x, int y, const double size = 1, const double angle = 0, SDL_Point *center = NULL, const SDL_RendererFlip flip = SDL_FLIP_NONE, const Uint8 transparency = 255, const rgb *colormod = NULL);
	};
	
	
	
	
	class texture_manager
	{
	private:
		plf::renderer *renderer;
		plf::atlas_manager *atlas_manager;
		int maximum_width, maximum_height;
	public:
		texture_manager(plf::renderer *p_renderer, plf::atlas_manager *p_atlas_manager);
		~texture_manager();
		
		// Automatically determine whether new texture needs to be a texture or multitexture:
		texture * add_image(SDL_Surface *new_surface);
	};

}

#endif // texture
