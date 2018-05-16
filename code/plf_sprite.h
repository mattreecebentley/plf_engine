#ifndef PLF_SPRITE_H
#define PLF_SPRITE_H

#include <vector>
#include <map>

#include <SDL2/SDL.h>

#include "plf_texture.h"
#include "plf_colony.h"


namespace plf
{

	enum HORIZONTAL_ALIGNMENT
	{
	   ALIGN_LEFT,
	   ALIGN_RIGHT,
	   ALIGN_CENTER
	};
	
	
	enum VERTICAL_ALIGNMENT
	{
	   ALIGN_TOP,
	   ALIGN_BOTTOM,
	   ALIGN_MIDDLE
	};
	
	
	
	enum LOOPING
	{
	   LOOP,
	   NO_LOOP,
	};
	
	
	
	
	
	class sprite
	{
	private:
		struct frame
		{
			std::vector<SDL_Rect> collision_blocks; // per-frame collision blocks (optional, not required)
			plf::texture *texture;
			unsigned int milliseconds;
			int adjust_x, adjust_y; // These adjust the placement of the frame, when dealing with dissimilar frame geometries
			int width, height;
			bool self_added;
		};
	
		// Note: frame numbers are from 1 upwards. unfortunately, vectors have array-like syntax (0 is first element)
		// This affects implementation in the functions below.
		std::vector <frame> frames;
		plf::texture_manager *texture_manager;
		SDL_Point *center;
		unsigned int total_sprite_time; // The total amount of milliseconds taken by all frames of the sprite
		unsigned int base_width, base_height;
		HORIZONTAL_ALIGNMENT horizontal_alignment;
		VERTICAL_ALIGNMENT vertical_alignment;
		bool loop;
		bool has_per_frame_collision_blocks; // Ie. collision blocks are being stored per-frame rather than in the parent entity
	public:
		sprite(plf::texture_manager *_texture_manager, bool _loop, HORIZONTAL_ALIGNMENT _horizontal_alignment, VERTICAL_ALIGNMENT _vertical_alignment);
		~sprite();
	
		// Note: Returns new current_sprite_time, which is the measurement in milliseconds of how far along the animation is:
		int draw(unsigned int &current_sprite_time, unsigned int delta_time, int x, int y, const double size = 1, const bool flip_horizontal = false, const bool flip_vertical = false, const double angle = 0, const Uint8 transparency = 255, rgb *colormod = NULL);
		int draw_frame(const unsigned int frame_number, int x, int y, const double size = 1, const bool flip_horizontal = false, const bool flip_vertical = false, const double angle = 0, const Uint8 transparency = 255, rgb *colormod = NULL);
		int update_frame(unsigned int &current_frame_number, unsigned int &current_sprite_time, const int delta, unsigned int &frame_time_remainder); // Based on delta time that has passed, update the sprite to whatever frame it should currently be on. frame_time_remainder allows the entity which uses the sprite to hold the current 'sprite time', rather than the sprite itself.
		int find_frame(const unsigned int current_sprite_time, unsigned int &current_frame_number, unsigned int &remainder);
	
		int add_frame(const char *image_filename, const unsigned int milliseconds);
		int add_frames(const char *image_filename_fragment, const unsigned int number_of_frames, const unsigned int milliseconds_per_frame);
		int add_frames_from_tile(const char *image_filename, const unsigned int number_of_frames, const unsigned int frame_width, const unsigned int milliseconds);
		int add_collision_block_to_frame(const unsigned int frame_number, const int x, const int y, const int w, const int h);
		void get_collision_blocks(const unsigned int frame_number, std::vector<SDL_Rect> &current_collision_blocks);
		int change_frame_timing(const unsigned int frame_number, const unsigned int milliseconds);
		int change_frame_texture(const char *image_filename, const unsigned int frame_number);
		int remove_frame(const unsigned int frame_number);
		void get_base_dimensions(int &width, int &height);
		unsigned int get_frame_timing(const unsigned int frame_number);
		bool has_collision_blocks() { return has_per_frame_collision_blocks; };
		bool has_frames() { return !(frames.empty()); };
		bool is_looping() { return loop; };
	};
	
	
	
	class sprite_manager
	{
	private:
		// All sprites used in game:
		std::map<std::string, sprite *> sprites;
	
		plf::texture_manager *texture_manager;
	public:
		sprite_manager(plf::texture_manager *_texture_manager);
		~sprite_manager();
		sprite * new_sprite(const std::string &id, const LOOPING loop = NO_LOOP, const HORIZONTAL_ALIGNMENT horiz_align = ALIGN_LEFT, const VERTICAL_ALIGNMENT vert_align = ALIGN_TOP); // Create a new sprite with the given ID and these settings
		sprite * get_sprite(const std::string &id); // Return the sprite with this ID.
		int remove_sprite(const std::string &id);
	};


}
#endif // sprite_H
