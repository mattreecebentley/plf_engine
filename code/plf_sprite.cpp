#include <cstdio>
#include <cassert>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "plf_utility.h"
#include "plf_texture.h"
#include "plf_sprite.h"
#include "plf_colony.h"


namespace plf
{


sprite::sprite(plf::texture_manager *_texture_manager, bool _loop, HORIZONTAL_ALIGNMENT _horizontal_alignment, VERTICAL_ALIGNMENT _vertical_alignment):
	texture_manager(_texture_manager),
	center(NULL),
	total_sprite_time(0),
	base_width(0),
	base_height(0),
	horizontal_alignment(_horizontal_alignment),
	vertical_alignment(_vertical_alignment),
	loop(_loop),
	has_per_frame_collision_blocks(false)
{
	assert(texture_manager != NULL);
}



sprite::~sprite()
{
	if (!frames.empty())
	{
		for(std::vector<frame>::iterator frame_iterator = frames.begin(); frame_iterator != frames.end(); ++frame_iterator)	
		{
			if (!(frame_iterator->self_added))
			{
				delete frame_iterator->texture;
			}
		}
	}
	
	delete center;
}



int sprite::draw(unsigned int &current_sprite_time, const unsigned int delta_time, int x, int y, const double size, const bool flip_horizontal, bool flip_vertical, double angle, const Uint8 transparency, rgb *colormod)
{
	if (transparency == 0)
	{
		return 0;
	}
	
	if (size <= 0) // sanity-check size parameter
	{
		return -1;
	}
	
	if (frames.empty()) // No frames loaded
	{
		return -1; // quit
	}
	
	std::vector<frame>::iterator current_frame = frames.begin();
	int return_value = 0;


	if (frames.size() != 1) // ie. If not a static 1-frame sprite, Choose frame based on current time:
	{
		current_sprite_time += delta_time;
		
		if (current_sprite_time > total_sprite_time) // time elapsed since last sprite display is greater than total animation loop time
		{
			if (loop)
			{
				current_sprite_time = current_sprite_time % total_sprite_time; // removes all full loops of sprite animation
			}
			else
			{
				current_sprite_time = total_sprite_time - 1; // Current_sprite_time is end_frame
				current_frame = frames.end();
				--current_frame;
				return_value = 20; // Indicates end-of-sprite.
			}
		}

		if (return_value != 20)
		{
			unsigned int frame_timing = 0;
			
			for(; current_frame != frames.end(); ++current_frame)
			{
				frame_timing += current_frame->milliseconds;

				if (frame_timing >= current_sprite_time)
				{
					break;
				}
			}
		}
	}

	HORIZONTAL_ALIGNMENT temp_horizontal_alignment = horizontal_alignment;
	VERTICAL_ALIGNMENT temp_vertical_alignment = vertical_alignment;
	
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	
	if (flip_horizontal)
	{
		flip = SDL_FLIP_HORIZONTAL;

		if (temp_horizontal_alignment == ALIGN_LEFT)
		{
			temp_horizontal_alignment = ALIGN_RIGHT;
		}
		else if (temp_horizontal_alignment == ALIGN_RIGHT)
		{
			temp_horizontal_alignment = ALIGN_LEFT;
		}
	}
	
	if (flip_vertical)
	{
		flip = SDL_RendererFlip(flip | SDL_FLIP_VERTICAL);

		if (temp_vertical_alignment == ALIGN_TOP)
		{
			temp_vertical_alignment = ALIGN_BOTTOM;
		}
		else if (temp_vertical_alignment == ALIGN_BOTTOM)
		{
			temp_vertical_alignment = ALIGN_TOP;
		}
	}
	

	center = NULL;
	
	// Adjust positioning and rotation centre to accomodate changed frame size and horizontal_alignment:
	if (current_frame->adjust_x != 0 || current_frame->adjust_y != 0)
	{
		// Adjust (potential) rotation center:
		center = new SDL_Point;

		if (temp_horizontal_alignment == ALIGN_LEFT)
		{
			center->x = base_width / 2;
		}
		else if (temp_horizontal_alignment == ALIGN_RIGHT)
		{
			x += current_frame->adjust_x;
			center->x = (base_width / 2) - current_frame->adjust_x;
		}
		else if (temp_horizontal_alignment == ALIGN_CENTER)
		{
			x += (current_frame->adjust_x / 2);
			center->x = current_frame->width / 2;
		}
		else
		{
			center->x = 0;
		}

		if (temp_vertical_alignment == ALIGN_TOP)
		{
			center->y = base_height / 2;
		}
		else if (temp_vertical_alignment == ALIGN_BOTTOM)
		{
			y += current_frame->adjust_y;
			center->y = (base_height / 2) - current_frame->adjust_y;
		}
		else if (temp_vertical_alignment == ALIGN_MIDDLE)
		{
			y += (current_frame->adjust_y / 2);
			center->y = current_frame->height / 2;
		}
		else
		{
			center->y = 0;
		}

		center->x += x;
		center->y += y;
	}

	current_frame->texture->draw(x, y, size, angle, center, flip, transparency, colormod);

	delete center;
	return return_value; // return the current sprite time to the entity
}



int sprite::draw_frame(const unsigned int frame_number, int x, int y, const double size, const bool flip_horizontal, const bool flip_vertical, const double angle, const Uint8 transparency, rgb *colormod)
{
	assert(frames.size() != 0);
	assert(frame_number < frames.size());
	assert(size > 0);
	
	std::vector<frame>::iterator current_frame = frames.begin() + frame_number;
	
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	
	if (flip_horizontal)
	{
		flip = SDL_FLIP_HORIZONTAL;

		if (horizontal_alignment == ALIGN_LEFT)
		{
			horizontal_alignment = ALIGN_RIGHT;
		}
		else if (horizontal_alignment == ALIGN_RIGHT)
		{
			horizontal_alignment = ALIGN_LEFT;
		}
	}
	
	if (flip_vertical)
	{
		flip = SDL_RendererFlip(flip | SDL_FLIP_VERTICAL);

		if (vertical_alignment == ALIGN_TOP)
		{
			vertical_alignment = ALIGN_BOTTOM;
		}
		else if (vertical_alignment == ALIGN_BOTTOM)
		{
			vertical_alignment = ALIGN_TOP;
		}
	}
	

	center = NULL;
	
	// Adjust positioning and rotation centre to accomodate changed frame size and horizontal_alignment:
	if (current_frame->adjust_x != 0 || current_frame->adjust_y != 0)
	{
		// Adjust (potential) rotation center:
		center = new SDL_Point;

		if (horizontal_alignment == ALIGN_LEFT)
		{
			center->x = base_width / 2;
		}
		else if (horizontal_alignment == ALIGN_RIGHT)
		{
			x += current_frame->adjust_x;
			center->x = (base_width / 2) - current_frame->adjust_x;
		}
		else if (horizontal_alignment == ALIGN_CENTER)
		{
			x += (current_frame->adjust_x / 2);
			center->x = current_frame->width / 2;
		}
		else
		{
			center->x = 0;
		}

		if (vertical_alignment == ALIGN_TOP)
		{
			center->y = base_height / 2;
		}
		else if (vertical_alignment == ALIGN_BOTTOM)
		{
			y += current_frame->adjust_y;
			center->y = (base_height / 2) - current_frame->adjust_y;
		}
		else if (vertical_alignment == ALIGN_MIDDLE)
		{
			y += (current_frame->adjust_y / 2);
			center->y = current_frame->height / 2;
		}
		else
		{
			center->y = 0;
		}

		center->x += x;
		center->y += y;
	}

	current_frame->texture->draw(x, y, size, angle, center, flip, transparency, colormod);
	
	delete center;
	return 0;
}



int sprite::update_frame(unsigned int &current_frame_number, unsigned int &current_sprite_time, int delta, unsigned int &frame_time_remainder)
{
	const unsigned int number_of_frames = static_cast<unsigned int>(frames.size());
	assert(number_of_frames != 0); // No frames loaded
	
	if (number_of_frames == 1) // Single-frame animation is inherently non-looping, end of frame
	{
		return 21; // Indicate single-frame animation
	}
	
	if (current_frame_number >= number_of_frames) // This shouldn't happen unless the user removes frames from the sprite in-game
	{
		current_frame_number = number_of_frames - 1; // Set equal to final frame
	}
	
	if (delta <= static_cast<int>(frame_time_remainder)) // Stay on current frame, reduce remainder
	{
		current_sprite_time += delta;
		frame_time_remainder -= delta;
		return 0;
	}
	

	// ie. If not a static 1-frame sprite, Choose frame based on current time:
	current_sprite_time += delta;

	if (current_sprite_time >= total_sprite_time) // time elapsed since last sprite display is greater than total animation loop time
	{
		if (loop)
		{
			unsigned int frame_timing = 0;
			current_sprite_time = current_sprite_time % total_sprite_time; // removes all full loops of sprite animation
			current_frame_number = 0; // Reset frame back to beginning frame
			std::vector<frame>::iterator current_frame = frames.begin();

			for(; current_frame_number != number_of_frames; ++current_frame_number)
			{
				frame_timing += current_frame->milliseconds;

				if (frame_timing >= current_sprite_time)
				{
					frame_time_remainder = frame_timing - current_sprite_time;
					return 2; // Indicates it has looped
				}
				
				++current_frame;
			}
		}
		else
		{
			current_frame_number = number_of_frames - 1;
			frame_time_remainder = (frames.begin() + current_frame_number)->milliseconds;
			current_sprite_time = total_sprite_time - frame_time_remainder; // Current_sprite_time is end_frame
			return 20; // Indicates end-of-sprite.
		}
	}


	// Progress to subsequent frame(s), remove remainder of this frame from delta
	delta -= frame_time_remainder; 
	++current_frame_number; // If next frame was already end, current_sprite_time would've been over total_sprite_time as established above.
	std::vector<frame>::iterator current_frame = frames.begin() + current_frame_number;
	
	for(; current_frame_number != number_of_frames; ++current_frame_number)
	{
		delta -= current_frame->milliseconds;

		if (delta < 0)
		{
			frame_time_remainder = -delta;
			return 0;
		}
		
		++current_frame;
	}
	
	std::clog << "plf::sprite update function Warning: Update function reached end without normal terminating conditions - something is wrong in code." << std::endl;
	return 0; // Not strictly necessary as all cases should be covered in above return statements
}



int sprite::find_frame(unsigned int current_sprite_time, unsigned int &current_frame_number, unsigned int &remainder)
{
	const unsigned int number_of_frames = static_cast<unsigned int>(frames.size());
	unsigned int frame_timing = 0;
	
	if (current_sprite_time > total_sprite_time)
	{
		current_sprite_time = current_sprite_time % total_sprite_time; // removes all full loops of sprite animation
	}
	
	current_frame_number = 0; // Reset frame back to beginning frame

	for(std::vector<frame>::iterator current_frame = frames.begin(); current_frame_number != number_of_frames; ++current_frame_number)
	{
		frame_timing += current_frame->milliseconds;

		if (frame_timing >= current_sprite_time)
		{
			remainder = frame_timing - current_sprite_time;
			return 0;
		}
		
		++current_frame;
	}
	
	return -1; // Could not find frame
}



int sprite::add_frame(const char *image_filename, const unsigned int milliseconds)
{
	frames.push_back(frame());
	frame &frame_pointer = frames.back();
	
	SDL_Surface *image_surface = IMG_Load(image_filename);
	
	plf_fail_if (image_surface == NULL, "plf::sprite add_frame Error: Unable to load image file '" << image_filename << "' to surface! SDL Error: " << SDL_GetError());
	
	frame_pointer.width = image_surface->w;
	frame_pointer.height = image_surface->h;
	frame_pointer.texture = texture_manager->add_image(image_surface);

	assert(frame_pointer.texture != NULL);

	SDL_FreeSurface(image_surface);

	frame_pointer.milliseconds = milliseconds;
	total_sprite_time += milliseconds;
	
	
	if (frames.size() == 1) // If this is the first frame, set the base width and height
	{
		base_width = frame_pointer.width;
		base_height = frame_pointer.height;
		frame_pointer.adjust_x = 0;
		frame_pointer.adjust_y = 0;
	}
	else
	{
		frame_pointer.adjust_x = base_width - frame_pointer.width;
		frame_pointer.adjust_y = base_height - frame_pointer.height;
	}
	
	return 0;
}



int sprite::add_frames(const char *image_filename_fragment, const unsigned int number_of_frames, const unsigned int milliseconds_per_frame)
{
	char image_filename [1024];
	SDL_Surface *image_surface;
	
	for (unsigned int frame_number = 1; frame_number != number_of_frames + 1; ++frame_number)
	{
		sprintf(image_filename, "%s%u.png", image_filename_fragment, frame_number);
		frames.push_back(frame());
		frame &frame_pointer = frames.back();
		
		image_surface = IMG_Load(image_filename);
		
		plf_fail_if (image_surface == NULL, "plf::sprite add_frames Error: Unable to load image file '" << image_filename << "' to surface! SDL Error: " << SDL_GetError());
		
		frame_pointer.texture = texture_manager->add_image(image_surface);
		frame_pointer.width = image_surface->w;
		frame_pointer.height = image_surface->h;

		SDL_FreeSurface(image_surface);
		image_surface = NULL;

		assert(frame_pointer.texture != NULL);

		frame_pointer.milliseconds = milliseconds_per_frame;
		total_sprite_time += milliseconds_per_frame;

		if (frames.size() == 1) // If this is the first frame, set the base width and height
		{
			base_width = frame_pointer.width;
			base_height = frame_pointer.height;
			frame_pointer.adjust_x = 0;
			frame_pointer.adjust_y = 0;
		}
		else
		{
			frame_pointer.adjust_x = base_width - frame_pointer.width;
			frame_pointer.adjust_y = base_height - frame_pointer.height;
		}

	}
	
	return 0;
}



int sprite::add_frames_from_tile(const char *image_filename, const unsigned int number_of_frames, const unsigned int frame_width, const unsigned int milliseconds)
{
	SDL_Surface *tiles_surface = IMG_Load(image_filename);
	plf_fail_if (tiles_surface == NULL, "plf::sprite add_frames_from_tile Error: Unable to load image file '" << image_filename << "' to surface. SDL Error: " << SDL_GetError());

	const int total_width = static_cast<int>(number_of_frames * frame_width);
	assert(tiles_surface->w == total_width); // Width of image not equal to specified number of frames * specified frame width.

    // Create a frame surface for copying the individual frames to:
	SDL_Surface *frame_surface = create_surface(frame_width, tiles_surface->h);

    plf_fail_if (frame_surface == NULL, "plf::sprite add_frames_from_tile Error: could not create temporary frame_surface.");

	SDL_SetSurfaceBlendMode(tiles_surface, SDL_BLENDMODE_NONE);
	
	SDL_Rect source_rectangle = {0, 0, static_cast<int>(frame_width), tiles_surface->h};

	for (; source_rectangle.x != total_width; source_rectangle.x += frame_width)
	{
		// Copy frame tile to it's own surface:
		plf_fail_if (SDL_BlitSurface(tiles_surface, &source_rectangle, frame_surface, NULL) < 0, "plf::sprite add_frames_from_tile Error: Unable to blit surface to surface. x = " << source_rectangle.x << ". SDL Error: " << SDL_GetError());

		frames.push_back(frame());
		frame &frame_pointer = frames.back();

		frame_pointer.texture = texture_manager->add_image(frame_surface);

		plf_fail_if (frame_pointer.texture == NULL, "plf::sprite add_frames_from_tile Error: Unable to create texture from framed surface. SDL Error: " << SDL_GetError());

		frame_pointer.width = frame_width;
		frame_pointer.height = source_rectangle.h;
		frame_pointer.milliseconds = milliseconds;
		total_sprite_time += milliseconds;

		if (frames.size() == 1) // If this is the first frame, set the base width and height
		{
			base_width = frame_pointer.width;
			base_height = frame_pointer.height;
			frame_pointer.adjust_x = 0;
			frame_pointer.adjust_y = 0;
		}
		else
		{
			frame_pointer.adjust_x = base_width - frame_pointer.width;
			frame_pointer.adjust_y = base_height - frame_pointer.height;
		}
	}
	
	SDL_FreeSurface(tiles_surface);
	SDL_FreeSurface(frame_surface);
	return 0;
}



int sprite::change_frame_timing(const unsigned int frame_number, const unsigned int milliseconds)
{
	assert(frame_number <= frames.size()); // ie. frame number is not higher than size of number of frames in sprite

	(frames.begin() + frame_number - 1)->milliseconds = milliseconds;
	return 0;
}



int sprite::change_frame_texture(const char *image_filename, const unsigned int frame_number)
{
	assert(frame_number <= frames.size()); // ie. frame number is not higher than size of number of frames in sprite
	
	frame &frame_pointer = *(frames.begin() + frame_number - 1);
	
	// Retain old texture link so that if new file can't be loaded, we don't end up with a NULL texture.
	texture *old_texture = frame_pointer.texture;
	frame_pointer.texture = NULL;

	// Load new texture:
	SDL_Surface *image_surface = IMG_Load(image_filename);
	plf_fail_if (image_surface == NULL, "plf::sprite change_frame_texture Error: Unable to load image file '" << image_filename << "' to surface! SDL Error: " << SDL_GetError());

	frame_pointer.texture = texture_manager->add_image(image_surface);
	plf_fail_if (frame_pointer.texture == NULL, "plf::sprite change_frame_texture Error: Unable to copy surface from '" << image_filename << "' to texture atlas! SDL Error: " << SDL_GetError());

	frame_pointer.width = image_surface->w;
	frame_pointer.height = image_surface->h;
	SDL_FreeSurface(image_surface);

	// Success - Remove prior texture:
	if (frame_pointer.self_added)
	{
		delete old_texture;
	}

	frame_pointer.self_added = true;
	return 0;
}



int sprite::remove_frame(const unsigned int frame_number)
{
	assert(frame_number <= frames.size()); // ie. frame number is not higher than size of number of frames in sprite
	assert(frame_number != 0);

	std::vector<frame>::iterator selected_frame = frames.begin() + frame_number - 1;

	if (selected_frame->self_added)
	{
		delete selected_frame->texture;
	}

	frames.erase(selected_frame);
	
	return 0;
}



int sprite::add_collision_block_to_frame(const unsigned int frame_number, const int x, const int y, const int w, const int h)
{
	assert(frame_number <= frames.size()); // ie. frame number is not higher than size of number of frames in sprite

	SDL_Rect temp_rect = {x, y, w, h};

	(frames.begin() + frame_number)->collision_blocks.insert(temp_rect);
	has_per_frame_collision_blocks = true;
	return 0;
}



void sprite::get_collision_blocks(const unsigned int frame_number, plf::colony<SDL_Rect> &current_collision_blocks)
{
	assert(frame_number <= frames.size());
	current_collision_blocks = (frames.begin() + frame_number)->collision_blocks;
}



void sprite::get_base_dimensions(int &width, int &height)
{
	width = static_cast<int>(base_width);
	height = static_cast<int>(base_height);
}



unsigned int sprite::get_frame_timing(const unsigned int frame_number)
{
	assert(frame_number <= frames.size());
	return (frames.begin() + frame_number)->milliseconds;
}



sprite_manager::sprite_manager(plf::texture_manager *_texture_manager):
	texture_manager(_texture_manager)
{
	assert(texture_manager != NULL);
}



sprite_manager::~sprite_manager()
{
	// Destroy Sprites:
	for (std::map<std::string, sprite *>::iterator sprite_iterator = sprites.begin(); sprite_iterator != sprites.end(); ++sprite_iterator)
	{
		delete sprite_iterator->second;
	}
}



sprite * sprite_manager::new_sprite(const std::string &id, const LOOPING looping, const HORIZONTAL_ALIGNMENT h_align, const VERTICAL_ALIGNMENT v_align)
{
	plf::sprite *sprite = new plf::sprite(texture_manager, (looping == LOOP), h_align, v_align);

	std::pair<std::map<std::string, plf::sprite *>::iterator, bool> return_value;
	return_value = sprites.insert(std::pair<std::string, plf::sprite *>(id, sprite));
	
	assert(return_value.second); // ie. an element with that id doesn't already exist
	
	return sprite;
}



sprite * sprite_manager::get_sprite(const std::string &id)
{
	std::map<std::string, sprite *>::iterator sprite_iterator = sprites.find (id);
	
	// if sprite doesn't exist, return null, otherwise:
	if (sprite_iterator == sprites.end())
	{
		return NULL;
	}
	
	return sprite_iterator->second;
}



int sprite_manager::remove_sprite(const std::string &id)
{
	std::map<std::string, sprite *>::iterator sprite_iterator = sprites.find (id);
	
	// if sprite doesn't exist, return -1:
	if (sprite_iterator == sprites.end())
	{
		return -1;
	}
	
	delete sprite_iterator->second;
	sprites.erase(sprite_iterator);
	return 0;
}


}
