#ifndef PLF_ENTITY_H
#define PLF_ENTITY_H

#include <string>
#include <map>

#include <SDL2/SDL.h>

#include "plf_sprite.h"
#include "plf_sound.h"
#include "plf_colony.h"
#include "plf_movement.h"
#include "plf_utility.h"


namespace plf
{

	struct entity_block; // Forward declaration - avoids circular dependencies with quadtree.h
	class quadtree; // ditto
	
	
	
	class entity
	{
	private:
		struct state
		{
			std::vector<SDL_Rect> collision_blocks;			 		// Collision blocks for state, may be overridden by sprite collision blocks
			plf::colony<sound_reference> sound_references;	 		// Any sounds associated with that state
			plf::sprite *sprite; 									// No sprite if == NULL
			plf::movement *movement;								// No movement if == NULL
			unsigned int remainder;									// Remainder of time left within current frame
			unsigned int current_sprite_time;						// Tracks time-placement within the sprite animation.
			unsigned int current_frame_number;						// Current sprite frame
			unsigned int current_movement_time;						// Tracks time-placement within the movement function. Starts at 0, loops at 2 thousand million
			bool self_destruct_on_sprite_end; 						// Indicates that at the end of this state's sprite, this entity should self-destruct (return 20)
		};
	
		std::map <std::string, state> states;
		plf::colony<entity_block *> current_quadtree_blocks;
		std::string id, type, current_state_id;
		SDL_Rect current_area; // Height and width match the base dimensions of the current state's sprite. Used with allowed_area below.
		plf::sound_manager * sound_manager; // Must be non-const in order for swap() to work
		quadtree *layer_quadtree;	// Pointer to the root node of the quadtree of the layer this entity has been spawned on... for use with adding back into quadtree upon move
		state *current_state;
		rgb *colormod;
		SDL_Rect *allowed_area; // If not NULL, outside of this area the entity will be destroyed automatically by the engine.
		double angle;
		double game_x, game_y; // Location of entity in game's greater x, y coordinates. Initially strict integers), as the game begins and the entity begins to move, the coordinates become non-integer.
		double size;
		unsigned int global_state_time_offset;
		bool flip_horizontal, flip_vertical;
		Uint8 transparency;
	
	public:
		entity(const std::string &entity_id, plf::sound_manager *_sound_manager);
		entity(const entity &source);
		entity(): colormod(NULL), allowed_area(NULL) { }; // For classes which inherit from entity - stops destructor on child entity from going mental
		virtual ~entity(); // Virtual only necessary because otherwise compiler complains, due to virtual update() below.
		void add_state(const std::string &id, sprite *sprite, const bool destruct_on_sprite_end = false);
		void add_sound_to_state(const std::string &state_id, const std::string &sound_id, const SOUND_REFERENCE_TYPE sound_type, const unsigned int delay_before_playing = 0, const unsigned int tween_delay = 0, const unsigned int tween_delay_random = 0);
		void add_collision_block_to_state(const std::string &state_id, const int x, const int y, const int w, const int h);
		template <class movement_type> void add_movement_to_state(const std::string &state_id);
		void set_current_state(const std::string &state_id);
		void set_sprite_time_offset(const unsigned int time_offset);
		void set_movement_time_offset(const unsigned int time_offset);
		void set_location(const double x, const double y);
		void get_location(double &x, double &y);
		void set_global_state_time_offset(const unsigned int offset);
		void set_size(const double resize);
		void set_allowed_area(const int x, const int y, const int w, const int h);
		void set_horizontal_flip(const bool new_flip);
		void set_vertical_flip(const bool new_flip);
		void set_angle(const double angle);
		void set_transparency(const Uint8 transparency);
		void set_quadtree(quadtree *quadtree_top_node);
		void set_color_modulation(const Uint8 r, const Uint8 g, const Uint8 b);
		void set_id(const std::string &new_id);
		void set_type(const std::string &new_id);
		bool test_boundary_collision(SDL_Rect *external_rect); // deprecated, quadtree class does all the collision work now
		void get_current_collision_blocks(std::vector<SDL_Rect> &current_collision_blocks);
		std::string get_id();
		std::string get_type();
		std::string get_current_state_id();
	
		virtual int update(const unsigned int delta_time); //Updates movement/location etc. Always returns 20 if the entity needs to be destroyed.
		virtual int move(const unsigned int delta_time); //Updates movement/location etc. Always returns 20 if the entity needs to be destroyed.
		int draw(const double display_x, const double display_y, const Uint8 transparency = 255, rgb *colormod = NULL);
		
		inline void add_quadtree_block(entity_block *block_to_add) { current_quadtree_blocks.insert(block_to_add); };
		
		void swap(entity &destination);
	
		inline entity& operator = (entity const& source)
		{
			entity temp_destination(source);
			temp_destination.swap(*this);
			return *this;
		};
	};
	
	
	
	// Must be defined here because it's a template and templates are weird and stupid in c++
	template <class movement_type> void entity::add_movement_to_state(const std::string &state_id)
	{
		std::map<std::string, state>::iterator state_iterator = states.find(state_id);
		plf_assert(state_iterator != states.end(), "plf::entity add_movement_to_state error: state '" << state_id << "' does not exist.");
	
		state *found_state = &(state_iterator->second);
		plf_assert(found_state->movement == NULL, "plf::entity add_movement_to_state error: state '" << state_id << "' already has movement assigned.");
	
		found_state->movement = new movement_type();
	}
	
	
	
	class entity_manager
	{
	private:
		// All entity prototypes used in game:
		std::map<std::string, entity *> entities;
	
		plf::sound_manager * const sound_manager;
	public:
		entity_manager(plf::sound_manager *_sound_manager);
		~entity_manager();
		entity * new_entity(const std::string &id);
		entity * get_entity(const std::string &id);
		int remove_entity(const std::string &id);
	};

}

#endif // entity_H
