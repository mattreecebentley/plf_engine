#include <cassert>
#include <utility> // std::pair

#include <SDL2/SDL.h>

#include "plf_atlas.h"
#include "plf_utility.h"
#include "plf_math.h"
#include "plf_texture.h"


namespace plf
{

texture::texture(plf::renderer *p_renderer, plf::atlas_manager *atlas_manager, SDL_Surface *surface):
	atlas_texture(NULL),
	atlas_coordinates(NULL),
	node(NULL),
	atlas(NULL)
{
	assert(p_renderer != NULL);
	assert(atlas_manager != NULL);

	s_renderer = p_renderer->get();
	p_renderer->get_dimensions(renderer_width, renderer_height);

	std::pair<plf::atlas *, plf::atlas_node *> atlas_pair;
	atlas_pair = atlas_manager->add_surface(surface);
	
	assert(atlas_pair.first != NULL && atlas_pair.second != NULL);
	
	atlas = atlas_pair.first;
	node = atlas_pair.second;
	
	atlas_texture = atlas->get_texture();
	atlas_coordinates = node->get_image_coordinates();
}



texture::~texture()
{
	atlas->remove_surface(node);
}



int texture::draw(int x, int y, const double size, const double angle, SDL_Point *center, const SDL_RendererFlip flip, const Uint8 transparency, const rgb *colormod)
{
	SDL_Rect render_destination = 
	{
		x, 
		y, 
		static_cast<int>(static_cast<double>(atlas_coordinates->w) * size), 
		static_cast<int>(static_cast<double>(atlas_coordinates->h) * size)
	};

	int return_value;

	if (transparency != 255) // 0 is covered in sprite call
	{
		SDL_SetTextureAlphaMod(atlas_texture, transparency);
	}

	if (colormod != NULL)
	{
		SDL_SetTextureColorMod(atlas_texture, colormod->r, colormod->g, colormod->b);
	}


	if (center == NULL)
	{
		// Optimise for most common scenario:
		if (angle == 0 && flip == SDL_FLIP_NONE && size == 1)
		{
			return_value = SDL_RenderCopy(s_renderer, atlas_texture, atlas_coordinates, &render_destination);
		}
		
		return_value = SDL_RenderCopyEx(s_renderer, atlas_texture, atlas_coordinates, &render_destination, angle, NULL, flip);
	}
	else
	{
		SDL_Point real_center = {center->x - x, center->y - y};
		return_value = SDL_RenderCopyEx(s_renderer, atlas_texture, atlas_coordinates, &render_destination, angle, &real_center, flip);
	}
	
	if (colormod != NULL)
	{
		SDL_SetTextureColorMod(atlas_texture, 255, 255, 255);
	}

	if (transparency != 255) // Reset texture atlas transparency for next texture
	{
		SDL_SetTextureAlphaMod(atlas_texture, 255);
	}

	return return_value;
}




multitexture::multitexture(renderer *p_renderer, atlas_manager *atlas_manager, SDL_Surface *surface, const unsigned int maximum_width, const unsigned int maximum_height):
	segments(NULL)
{
	assert(p_renderer != NULL);
	
	s_renderer = p_renderer->get();
	p_renderer->get_dimensions(renderer_width, renderer_height);
	
	// Divide texture into segments (remember, integer division always rounds down in c++):
	total_width = surface->w;
	total_height = surface->h;
	const unsigned int num_cells_x = divide_and_round_up(surface->w, maximum_width);
	const unsigned int num_cells_y = divide_and_round_up(surface->h, maximum_height);
	const unsigned int total_cells = num_cells_x * num_cells_y;
	segments = new segment[total_cells];
	end_segment = &(segments[total_cells]);
	segment *current_segment = &(segments[0]);

	// Initialise nodes to NULL just in case something goes wrong and a destructor call causes a segfault:
	for (; current_segment != end_segment; ++current_segment) 
	{
		current_segment->node = NULL;
	}

	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
	SDL_Surface *temp_surface;
	SDL_Rect source_rect;
	source_rect.w = maximum_width;
	source_rect.h = maximum_height;
	current_segment = &(segments[0]);

	std::pair<plf::atlas *, plf::atlas_node *> atlas_pair;

	// Break down surface into segments.
	// Note: segments overlap by 1pix vertically and horizontally to remove gaps when rotating. Hence the (maximum_height - 1) and ditto maximum_width in loops:
	for (source_rect.y = 0; source_rect.y < surface->h; source_rect.y += (maximum_height - 1))
	{
		source_rect.w = maximum_width;

		if ((surface->h - source_rect.y) < static_cast<int>(maximum_height)) // ie. if this is the final y segmentation and the size is lower than maximum_height
		{
			// Reduce the size of the created surface:
			source_rect.h = surface->h - source_rect.y;
		}

		temp_surface = create_surface(source_rect.w, source_rect.h);
		
		plf_fail_if (temp_surface == NULL, "plf::multitexture error: create_surface with width/height = " << source_rect.w << "/" << source_rect.h << " returned NULL. SDL_Error:" << SDL_GetError());

		for (source_rect.x = 0; source_rect.x < surface->w; source_rect.x += (maximum_width - 1))
		{
			if ((surface->w - source_rect.x) < static_cast<int>(maximum_width)) // ie. if this is the final x segmentation for this y run and the size is lower than maximum_width
			{
				// Reduce the size of the created surface:
				source_rect.w = surface->w - source_rect.x;
				SDL_FreeSurface(temp_surface);
				temp_surface = create_surface(source_rect.w, source_rect.h);
				
				plf_fail_if (temp_surface == NULL, "plf::multitexture error: create_surface with width/height = " << source_rect.w << "/" << source_rect.h << " returned NULL. SDL_Error:" << SDL_GetError());
			}

			plf_fail_if (SDL_BlitSurface(surface, &source_rect, temp_surface, NULL) < 0, "plf::multitexture error: Blitsurface failed. SDL_Error:" << SDL_GetError());
			
			atlas_pair = atlas_manager->add_surface(temp_surface);
			
			assert(atlas_pair.first != NULL && atlas_pair.second != NULL);
			
			current_segment->atlas = atlas_pair.first;
			current_segment->atlas_texture = current_segment->atlas->get_texture();
			current_segment->node = atlas_pair.second;
			current_segment->atlas_coordinates = current_segment->node->get_image_coordinates();
			current_segment->segment_x = source_rect.x;
			current_segment->segment_y = source_rect.y;
			++current_segment;
		}
		
		SDL_FreeSurface(temp_surface);
	}
}



multitexture::~multitexture()
{
	for (segment *current_segment = &(segments[0]); current_segment != end_segment; ++current_segment) // initialise nodes to NULL just in case something goes wrong and then the destructor call ruins everything.
	{
		if (current_segment->node != NULL)
		{
			current_segment->atlas->remove_surface(current_segment->node);
		}
	}

	delete [] segments;
}



int multitexture::draw(int x, int y, const double size, const double angle, SDL_Point *center, const SDL_RendererFlip flip, const Uint8 transparency, const rgb *colormod)
{
	int resized_width = static_cast<int>(static_cast<double>(total_width) * size);
	int resized_height = static_cast<int>(static_cast<double>(total_height) * size);
	int return_value = 0;


	// If not rotating, rule out entire image not being displayed onscreen):
	if (angle == 0)
	{ 
		if (x + resized_width < 0 || x > renderer_width || y + resized_height < 0 || y > renderer_height)
		{
			return 0;
		}
		
		// Optimise for most common scenario:
		if (flip == SDL_FLIP_NONE && size == 1 && transparency == 255 && colormod == NULL)
		{
			// Optimised draw loop:
			for (segment *current_segment = &(segments[0]); current_segment != end_segment; ++current_segment) // initialise nodes to NULL just in case something goes wrong and then the destructor call ruins everything.
			{
				SDL_Rect render_destination = 
				{
					x + current_segment->segment_x, 
					y + current_segment->segment_y, 
					current_segment->atlas_coordinates->w, 
					current_segment->atlas_coordinates->h
				};

				// Check whether sprite needs to be displayed at all - saves CPU-time - thoroughly benchmarked.
				// Self-cropping images to renderer frame that need to be displayed does not save CPU time -  SDL does it faster.
				if (!(render_destination.x + render_destination.w < 0 || render_destination.x > renderer_width || render_destination.y + render_destination.h < 0 || render_destination.y > renderer_height))
				{
					return_value += SDL_RenderCopy(s_renderer, current_segment->atlas_texture, current_segment->atlas_coordinates, &render_destination);
				}
			}
			
			return return_value;
		}
	}


	bool created_center = false;

	if (angle != 0 && center == NULL)
	{
		created_center = true;
		center = new SDL_Point;
		center->x = static_cast<int>((total_width / 2) * size) + x;
		center->y = static_cast<int>((total_height / 2) * size) + y;
	}

	double x_rotated, y_rotated;


	if (flip != SDL_FLIP_NONE)
	{
		if (flip & SDL_FLIP_HORIZONTAL)
		{
			x += resized_width;
		}

		if (flip & SDL_FLIP_VERTICAL)
		{
			y += resized_height;
		}
	}

	SDL_Rect render_destination;
	SDL_Point half_segment_size = {(segments[0].atlas_coordinates->w - 1) / 2, (segments[0].atlas_coordinates->h - 1) / 2};

	for (segment *current_segment = &(segments[0]); current_segment != end_segment; ++current_segment) // initialise nodes to NULL just in case something goes wrong and then the destructor call ruins everything.
	{
		render_destination.w = static_cast<int>(current_segment->atlas_coordinates->w * size);
		render_destination.h = static_cast<int>(current_segment->atlas_coordinates->h * size);
		render_destination.x = x + static_cast<int>(current_segment->segment_x * size);
		render_destination.y = y + static_cast<int>(current_segment->segment_y * size);

		if (flip != SDL_FLIP_NONE)
		{
			if (flip & SDL_FLIP_HORIZONTAL)
			{
				render_destination.x = (x - static_cast<int>(current_segment->segment_x * size)) - render_destination.w;
			}

			if (flip & SDL_FLIP_VERTICAL)
			{
				render_destination.y = (y - static_cast<int>(current_segment->segment_y * size)) - render_destination.h;
			}
		}

		if (angle != 0)
		{
			x_rotated = static_cast<double>(render_destination.x);
			y_rotated = static_cast<double>(render_destination.y);

			rotate_point_around_pivot(x_rotated, y_rotated, static_cast<double>(center->x), static_cast<double>(center->y), angle);
			
			render_destination.x = round_double_to_int(x_rotated);
			render_destination.y = round_double_to_int(y_rotated);
		}
		
		
		// Check whether sprite needs to be displayed at all - saves CPU-time - thoroughly benchmarked.
		// Self-cropping images to renderer frame that need to be displayed does not save CPU time -  SDL does it faster.
		if (!(render_destination.x + render_destination.w < 0 || render_destination.x > renderer_width || render_destination.y + render_destination.h < 0 || render_destination.y > renderer_height))
		{
			if (transparency != 255)
			{
				SDL_SetTextureAlphaMod(current_segment->atlas_texture, transparency);
			}

			if (colormod != NULL)
			{
				SDL_SetTextureColorMod(current_segment->atlas_texture, colormod->r, colormod->g, colormod->b);
			}

			return_value += SDL_RenderCopyEx(s_renderer, current_segment->atlas_texture, current_segment->atlas_coordinates, &render_destination, angle, &(half_segment_size), flip);

			if (colormod != NULL)
			{
				SDL_SetTextureColorMod(current_segment->atlas_texture, 255, 255, 255);
			}

			if (transparency != 255)
			{
				SDL_SetTextureAlphaMod(current_segment->atlas_texture, 255);
			}
		}
	}
	
	if (created_center) // == true
	{
		delete center;
	}

	return return_value;
}





texture_manager::texture_manager(plf::renderer *p_renderer, plf::atlas_manager *p_atlas_manager):
	renderer(p_renderer),
	atlas_manager(p_atlas_manager)
{
	assert(renderer != NULL);
	assert(atlas_manager != NULL);
	
	atlas_manager->get_maximum_texture_size(maximum_width, maximum_height);
	assert(maximum_width != 0 && maximum_height != 0); // Should not happen
}



texture_manager::~texture_manager()	
{
}



texture * texture_manager::add_image(SDL_Surface *new_surface)
{
	assert(new_surface != NULL);
	assert(new_surface->w > 0);
	assert(new_surface->h > 0);

	if (new_surface->w <= maximum_width && new_surface->h <= maximum_height) // Normal case
	{
		return new texture(renderer, atlas_manager, new_surface);
	}
	else // surface larger than maximum hardware texture dimensions, have to split up into multitexture:
	{
		return new multitexture(renderer, atlas_manager, new_surface, maximum_width, maximum_height);
	}
}



}