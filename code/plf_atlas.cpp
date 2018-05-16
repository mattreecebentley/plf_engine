#include <vector>
#include <cassert>

#include <SDL2/SDL.h>


#include "plf_math.h"
#include "plf_utility.h"
#include "plf_renderer.h"
#include "plf_atlas.h"


namespace plf
{

	atlas_node::atlas_node(const unsigned int node_x, const unsigned int node_y, const unsigned int node_width, const unsigned int node_height,atlas_node *parent):
		image_rect(NULL),
		parent_node(parent),
		split_a(NULL),
		split_b(NULL),
		x(node_x),
		y(node_y),
		width(node_width),
		height(node_height)
	{
	}
	
	
	
	atlas_node::~atlas_node()
	{
		delete split_a;
		delete split_b;
		delete image_rect;
	}
	
	
	
	atlas_node * atlas_node::add(const unsigned int image_width, const unsigned int image_height)
	{
		if (split_a != NULL) // if node has been split
		{
			atlas_node *return_node;
	
			return_node = split_a->add(image_width, image_height);
	
			if (return_node == NULL) // didn't find any convenient spaces in split_a
			{
				return_node = split_b->add(image_width, image_height);
			}
	
			return return_node;
		}
	
	
		if (image_rect != NULL || image_width > width || image_height > height)
		{
			// Unavailable or image is wider/taller than this node, retreat back up the tree:
			return NULL;
		}
	
	
		if (image_width == width && image_height == height)
		{
			// Use this node:
	
			image_rect = new SDL_Rect;
			image_rect->x = x;
			image_rect->y = y;
			image_rect->w = image_width;
			image_rect->h = image_height;
	
			return this;
		}
	
	
		if (width - image_width > height - image_height)
		{
			  /* extend to the right */
			  split_a = new atlas_node(x, y, image_width, height, this);
			  split_b = new atlas_node(x + image_width, y, width - image_width, height, this);
		}
		else
		{
			  /* extend to bottom */
			  split_a = new atlas_node(x, y, width, image_height, this);
			  split_b = new atlas_node(x, y + image_height, width, height - image_height, this);
		}
	
		 return split_a->add(image_width, image_height);
	}
	
	
	
	SDL_Rect * atlas_node::get_node_coordinates()
	{
		SDL_Rect *node_rect = new SDL_Rect;
	
		node_rect->x = static_cast<int>(x);
		node_rect->y = static_cast<int>(y);
		node_rect->w = static_cast<int>(width);
		node_rect->h = static_cast<int>(height);
		
		return node_rect;
	}
	
	
	
	SDL_Rect * atlas_node::get_image_coordinates()
	{
		return image_rect;
	}
	
	
	
	void atlas_node::consolidate_empty_children()
	{
		if (split_a->node_and_child_nodes_are_empty() && split_b->node_and_child_nodes_are_empty())
		{
			delete split_a;
			delete split_b;
		}
	}
	
	
	
	bool atlas_node::node_and_child_nodes_are_empty()
	{
		if (image_rect != NULL)
		{
			return false;
		}
		
		if (split_a != NULL)
		{
			return (split_a->node_and_child_nodes_are_empty() && split_b->node_and_child_nodes_are_empty());
		}
	
		return true;
	}
	
	
	
	
	
	atlas::atlas(plf::renderer *p_renderer, const unsigned int atlas_width, const unsigned int atlas_height):
		renderer(p_renderer)
	{
		assert(renderer != NULL);
		assert(atlas_width != 0);
		assert(atlas_height != 0);
	
		atlas_texture = SDL_CreateTexture(renderer->get(), renderer->get_texture_pixel_format(), SDL_TEXTUREACCESS_STATIC, atlas_width, atlas_height);
		plf_fail_if (atlas_texture == NULL, "plf::atlas initialisation Error: Unable to create texture of size " << atlas_width << "/" << atlas_height << ". ");
	
		SDL_SetTextureBlendMode(atlas_texture, SDL_BLENDMODE_BLEND);
		prime_node = new atlas_node(0, 0, atlas_width, atlas_height, NULL);
	}
	
	
	
	atlas::~atlas()
	{
		delete prime_node;
		SDL_DestroyTexture(atlas_texture);
	}
	
	
	
	
	atlas_node * atlas::add_surface(SDL_Surface *new_surface)
	{
		assert(new_surface != NULL);
	
		const int width = new_surface->w;
		const int height = new_surface->h;
		
		atlas_node *located_position = NULL;
		located_position = prime_node->add(width, height);
		
		if (located_position == NULL) // Location with enough space not found within surface
		{
			return NULL; // Totally valid behaviour (except for a newly-created atlas with no images in it - no supplied images should be larger than can fit in an atlas by the texture manager) - manager will create/select another atlas texture
		}
		
		SDL_Rect *image_coordinates_within_atlas = located_position->get_image_coordinates();
		SDL_SetSurfaceBlendMode(new_surface, SDL_BLENDMODE_NONE);
		SDL_Surface *surface = new_surface;
		
		if (new_surface->format->format != renderer->get_surface_pixel_format())
		{
			// Convert surface format to texture format:
			surface = create_surface(width, height);
	
			plf_fail_if (surface == NULL, "plf::atlas add_surface Error: Unable to create surface of size " << width << "/" << height << ". ");
	
			// Convert new_surface to texture pixel format:
			plf_fail_if (SDL_ConvertPixels(width, height, new_surface->format->format, new_surface->pixels, new_surface->pitch, renderer->get_surface_pixel_format(), surface->pixels, surface->pitch) < 0, "plf::atlas add_surface Error: Unable to convert surface of size " << width << "/" << height << ". ");
	
			SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
		}
	
		int return_value;
	
		if (SDL_MUSTLOCK(surface))
		{
			SDL_LockSurface(surface);
			return_value = SDL_UpdateTexture(atlas_texture, image_coordinates_within_atlas, surface->pixels, surface->pitch);
			SDL_UnlockSurface(surface);
		}
		else
		{
			return_value = SDL_UpdateTexture(atlas_texture, image_coordinates_within_atlas, surface->pixels, surface->pitch);
		}
	
		// If surface had to be converted to texture format, delete temp intermediate surface:
		if (surface != new_surface)
		{
			SDL_FreeSurface(surface);
		}
	
		plf_fail_if (return_value < 0, "plf::atlas add_surface Error: Unable to copy surface of size " << width << "/" << height << " to atlas texture. ");
	
		return located_position;
	}
	
	
	
	void atlas::remove_surface(atlas_node *node)
	{
		assert(node != NULL);
		assert(node->image_rect != NULL); // attempt to delete image in node where image doesn't exist - shouldn't happen
		
		delete node->image_rect;
		node->image_rect = NULL;
		atlas_node *parent = node->parent_node;
		bool all_empty = true;
		
		// consolidate empty nodes - traverse backwards up the tree recursively:
		while (all_empty && parent != NULL)
		{
			if (parent->split_a == node) // If current node is split_a of parent, investigate it's twin, & vice-versa
			{
				all_empty = parent->split_b->node_and_child_nodes_are_empty();
			}
			else
			{
				all_empty = parent->split_a->node_and_child_nodes_are_empty();
			}
			
			if (all_empty)
			{
				delete parent->split_a;
				delete parent->split_b;
				parent->split_a = NULL;
				parent->split_b = NULL;
			}
	
			node = parent;
			parent = node->parent_node;
		}
	}
	
	
	
	SDL_Texture * atlas::get_texture()
	{
		return atlas_texture;
	}
	
	
	
	atlas_manager::atlas_manager(plf::renderer *_renderer):
		renderer(_renderer)
	{
		assert(renderer != NULL);
	
		SDL_RendererInfo s_renderer_info = renderer->get_info();
		maximum_width = s_renderer_info.max_texture_width;
		maximum_height = s_renderer_info.max_texture_height;
		int renderer_w, renderer_h;
		renderer->get_dimensions(renderer_w, renderer_h);
	
		// If actual renderer resolution is smaller than maximum texture resolution, set atlas size to renderer resolution.
		// This prevents atlas sizes from being extremely large, which would also chew through main system memory under a directx situation.
		// (directx reflects texture memory in main memory, so that switching out from a program and releasing texture memory for other applications is possible)
		if (maximum_width > renderer_w)
		{
			maximum_width = renderer_w;
		}
	
		if (maximum_height > renderer_h)
		{
			maximum_height = renderer_h;
		}
		
		
		// If maximum dimension for textures (as established by the renderer) is not a power of two, round it down to one 
		// (most hardware performs better with power-of-two-sized textures, moreso with older hardware).
		if (!is_power_of_two(maximum_width))
		{
			maximum_width = round_down_to_power_of_two(static_cast<unsigned int>(maximum_width));
		}
		if (!is_power_of_two(maximum_height))
		{
			maximum_height = round_down_to_power_of_two(static_cast<unsigned int>(maximum_height));
		}
	
		atlas *first_atlas = new atlas(renderer, maximum_width, maximum_height);
		atlases.push_back(first_atlas);
	}
		
		
	
	atlas_manager::~atlas_manager()
	{
		for (std::vector<atlas *>::iterator current_atlas = atlases.begin(); current_atlas != atlases.end(); ++current_atlas)
		{
			delete *current_atlas;
		}
		
		// vector itself is allocated statically and will be automatically deallocated at destruction of atlas_manager entity.
	}
	
	
	
	std::pair<atlas *, atlas_node *> atlas_manager::add_surface(SDL_Surface *new_surface)
	{
		assert(new_surface->w <= maximum_width && new_surface->h <= maximum_height); // surface is not larger than maximum atlas width or height
	
		atlas_node *selected_node = NULL;
		atlas *selected_atlas = NULL;
			
	
		// Rule out Special-case (surface is exact size of a new atlas) - if special-case, new atlas will be create in code after:
		if (new_surface->w != maximum_width || new_surface->h != maximum_height)
		{
			for (std::vector<atlas *>::iterator current_atlas = atlases.begin(); current_atlas != atlases.end(); ++current_atlas)
			{
				// brackets needed to denote that the dereference applies only to the iterator and not to some pointer returned by a function that the iterator is accessing.
				selected_node = (*current_atlas)->add_surface(new_surface);
	
				if (selected_node != NULL)
				{
					// Brackets not needed here because context is clear (no function)
					selected_atlas = *current_atlas;
					break;
				}
			}
		}
	
	
		if (selected_node == NULL)
		{
			// Create a new atlas to house the image:
			atlas *new_atlas = new atlas(renderer, maximum_width, maximum_height);
			selected_node = new_atlas->add_surface(new_surface);
	
			assert(selected_node != NULL); // New atlas could not contain new surface, for some reason - this should not happen unless the system is out of video card memory storage
	
			selected_atlas = new_atlas;
			atlases.push_back(new_atlas);
		}
	
		return std::pair<atlas *, atlas_node *> (selected_atlas, selected_node);
	}
	
	
	
	SDL_Texture * atlas_manager::get_atlas_texture(const unsigned int atlas_number)
	{
		assert(atlas_number != 0);
		assert(atlas_number <= atlases.size()); // atlas number supplied is greater than total number of atlases
	
		return atlases[atlas_number - 1]->get_texture();
	}
	
	
	
	void atlas_manager::get_maximum_texture_size(int &width, int &height)
	{
		width = static_cast<int>(maximum_width);
		height = static_cast<int>(maximum_height);
	}

}
