// TODO: change sprite class so it has an update function,
// 		 and so the entity class takes back the remainder time on current frame and checks it against delta to see if an update is necessary.
//		 Also, entity class retains current frame.

#include <string>
#include <map>
#include <vector>
#include <cassert>
#include <algorithm>

#include <SDL2/SDL.h>

#include "plf_sprite.h"
#include "plf_entity.h"
#include "plf_sound.h"
#include "plf_quadtree.h"
#include "plf_movement.h"
#include "plf_utility.h"
#include "plf_colony.h"



namespace plf
{


entity::entity(const std::string &entity_id, plf::sound_manager *_sound_manager):
	id(entity_id),
	sound_manager(_sound_manager),
	layer_quadtree(NULL),
	current_state(NULL),
	colormod(NULL),
	allowed_area(NULL),
	angle(0),
	game_x(0),
	game_y(0),
	size(1),
	global_state_time_offset(0),
	flip_horizontal(false),
	flip_vertical(false),
	transparency(255)
{
	current_area.x = 0;
	current_area.y = 0;
	current_area.w = 0;
	current_area.h = 0;

	assert(sound_manager != NULL);
}


entity::entity(const entity &source):
	states(source.states),
	current_quadtree_blocks(source.current_quadtree_blocks),
	id(source.id),
	sound_manager(source.sound_manager),
	layer_quadtree(source.layer_quadtree),
	current_state(NULL),
	allowed_area(source.allowed_area),
	angle(source.angle),
	game_x(source.game_x),
	game_y(source.game_y),
	size(source.size),
	global_state_time_offset(source.global_state_time_offset),
	flip_horizontal(source.flip_horizontal),
	flip_vertical(source.flip_vertical),
	transparency(source.transparency)
{
	current_area.x = source.current_area.x;
	current_area.y = source.current_area.y;
	current_area.w = source.current_area.w;
	current_area.h = source.current_area.h;

	set_current_state(source.current_state_id);

	if (source.colormod == NULL)
	{
		colormod = NULL;
	}
	else
	{
		colormod = new rgb;
		colormod->r = source.colormod->r;
		colormod->g = source.colormod->g;
		colormod->b = source.colormod->b;
	}
	
	
	std::map<std::string, state>::iterator new_state_iterator = states.begin();
	
	for (std::map<std::string, state>::const_iterator state_iterator = source.states.begin(); state_iterator != source.states.end(); ++state_iterator)
	{
		new_state_iterator->second.movement = NULL;

		if (state_iterator->second.movement != NULL)
		{
			new_state_iterator->second.movement = state_iterator->second.movement->clone();
		}

		++new_state_iterator;
	}
}



void entity::swap(entity &destination)
{
	destination.id = id;
	destination.sound_manager = sound_manager;
	destination.current_quadtree_blocks = current_quadtree_blocks;
	destination.layer_quadtree = layer_quadtree;
	destination.game_x = game_x;
	destination.game_y = game_y;
	destination.angle = angle;
	destination.flip_horizontal = flip_horizontal;
	destination.flip_vertical = flip_vertical;
	destination.global_state_time_offset = global_state_time_offset;
	destination.size = size;
	destination.allowed_area = allowed_area;
	destination.current_area.x = current_area.x;
	destination.current_area.y = current_area.y;
	destination.current_area.w = current_area.w;
	destination.current_area.h = current_area.h;
	destination.transparency = transparency;
	
	if (colormod == NULL)
	{
		destination.colormod = NULL;
	}
	else
	{
		destination.colormod = new rgb;
		destination.colormod->r = colormod->r;
		destination.colormod->g = colormod->g;
		destination.colormod->b = colormod->b;
	}

	destination.states = states;
	destination.current_state = NULL;
	destination.set_current_state(current_state_id);
}



entity::~entity()
{
	// Destroy any state solid area Rect's:
	delete allowed_area;
	delete colormod;
}



void entity::add_state(const std::string &id, sprite *sprite, const bool destruct_on_sprite_end)
{
	assert(sprite != NULL);
	assert(sprite->has_frames());

	std::pair<std::map<std::string, state>::iterator, bool> return_value;
	return_value = states.insert(std::pair<std::string, state> (id, state()));

	plf_assert(return_value.second == true, "plf::entity: add_state error: state with id '" << id << "' not able to be inserted.");

	state *new_state = &(return_value.first->second);

	new_state->sprite = sprite;
	new_state->current_sprite_time = 0;
	new_state->current_movement_time = 0;
	new_state->movement = NULL;
	new_state->self_destruct_on_sprite_end = destruct_on_sprite_end;
	new_state->current_frame_number = 0;
	new_state->remainder = new_state->sprite->get_frame_timing(0);

	if (states.size() == 1) // If this is the first state added, set the entity's 'current' values
	{
		current_state = new_state;
		current_state->sprite->get_base_dimensions(current_area.w, current_area.h);
	}
}



void entity::add_sound_to_state(const std::string &state_id, const std::string &sound_id, const SOUND_REFERENCE_TYPE sound_type, const unsigned int delay_before_playing, const unsigned int tween_delay, const unsigned int tween_delay_random)
{
	std::map<std::string, state>::iterator found_state_iterator = states.find(state_id);
	plf_assert(found_state_iterator != states.end(), "plf::entity: add sound to state error: state with id '" << state_id << "' not found.");

	state *state_to_add_to = &(found_state_iterator->second);

	plf::sound *sound_to_use = sound_manager->get_sound(sound_id);
	plf_assert(sound_to_use != NULL, "plf::entity: add sound to state error: sound not found.");

	state_to_add_to->sound_references.insert(sound_reference(sound_manager, sound_to_use, sound_type, delay_before_playing, tween_delay, tween_delay_random));
}



void entity::add_collision_block_to_state(const std::string &state_id, const int x, const int y, const int w, const int h)
{
	std::map<std::string, state>::iterator found_state_iterator = states.find(state_id);

	plf_assert(found_state_iterator != states.end(), "plf::entity: add sound to state error: state with id '" << state_id << "' not found. Quitting");

	SDL_Rect temp_rect = {x, y, w, h};

	found_state_iterator->second.collision_blocks.insert(temp_rect);
}



void entity::set_current_state(const std::string &state_id)
{
	if (state_id == current_state_id)
	{
		return;
	}
	
	std::map<std::string, state>::iterator state_iterator = states.find(state_id);
	assert(state_iterator != states.end()); // No such state exists
	
	if (current_state != NULL) // Usually when a copy of an entity is made
	{
		// Reset current state before changing to new state:
		current_state->current_sprite_time = 0; 								// Reset current sprite time
		current_state->current_movement_time = 0; 								// Reset current state's movement time
		current_state->current_frame_number = 0;								// Reset frame
		current_state->remainder = current_state->sprite->get_frame_timing(0);	// Reset remainder
		
		if (!(current_state->sound_references.empty()))
		{
			for (plf::colony<sound_reference>::iterator reference_iterator = current_state->sound_references.begin(); reference_iterator != current_state->sound_references.end(); ++reference_iterator)
			{
				reference_iterator->stop();
			}
		}

		if (current_state->sprite->is_looping()) // Global time offset is only added if it's a looping sprite
		{
			current_state->current_sprite_time += global_state_time_offset;
			current_state->current_movement_time += global_state_time_offset;
		}
	}

	
	// Change state:
	current_state = &(state_iterator->second);
	current_state_id = state_id;
	current_state->sprite->get_base_dimensions(current_area.w, current_area.h);

	
	if (layer_quadtree != NULL) // If this instantiation isn't part of a cloning operation
	{
		// Destroy current collision blocks:
		plf::colony<quadtree *> used_nodes;
		quadtree *parent_node;
		bool detected;

		for (plf::colony<entity_block *>::iterator block_iterator = current_quadtree_blocks.begin(); block_iterator != current_quadtree_blocks.end(); ++block_iterator)
		{
			(*block_iterator)->entity_reference = this; // current solution for account for pointer invalidation by layer's entity vector resizing when entities added/removed - better solution might be to use unique numbers for each entity per layer
			parent_node = (*block_iterator)->parent_node;
			detected = false;
			
			for (plf::colony<quadtree *>::iterator used_nodes_iterator = used_nodes.begin(); used_nodes_iterator != used_nodes.end(); ++used_nodes_iterator)
			{
				if (*used_nodes_iterator == parent_node)
				{
					detected = true;
					break;
				}
			}
			
			if (!detected)
			{
				used_nodes.insert(parent_node);
				parent_node->delete_entity(this);
				parent_node->consolidate_node();
			}
		}

		// Initialise new state's collision blocks (if any):
		layer_quadtree->add_entity(this);
	}
	
}



void entity::set_location(const double x, const double y)
{
	game_x = x;
	game_y = y;
	
}


void entity::set_sprite_time_offset(const unsigned int time_offset)
{
	assert(current_state != NULL);

	current_state->current_sprite_time = time_offset;
	current_state->sprite->find_frame(current_state->current_sprite_time, current_state->current_frame_number, current_state->remainder);
}


void entity::set_movement_time_offset(const unsigned int time_offset)
{
	assert(current_state != NULL);

	current_state->current_movement_time = time_offset;
}


void entity::set_global_state_time_offset(const unsigned int offset)
{
	global_state_time_offset = offset;
}


void entity::set_size(const double new_size)
{
	// Sanity check:
	if (new_size <= 0)
	{
		size = 0.1f;
		return;
	}
	
	size = new_size;
}
	
	
void entity::set_allowed_area(const int x, const int y, const int w, const int h)
{
	if (allowed_area == NULL)
	{
		allowed_area = new SDL_Rect;
	}

	allowed_area->x = x;
	allowed_area->y = y;
	allowed_area->w = w;
	allowed_area->h = h;
}


void entity::set_quadtree(quadtree *quadtree_top_node)
{
	layer_quadtree = quadtree_top_node;
}


void entity::set_horizontal_flip(const bool new_flip)
{
	flip_horizontal = new_flip;
}
	

void entity::set_vertical_flip(const bool new_flip)
{
	flip_vertical = new_flip;
}

	
void entity::set_angle(const double new_angle)
{
	// Sanity checks:
	if (new_angle < 0)
	{
		angle = 360 + new_angle;
		return;
	}
	else if (new_angle > 360)
	{
		angle = new_angle - 360;
		return;
	}
	
	angle = new_angle;
}



void entity::set_id(const std::string &new_id)
{
	id = new_id;
}



void entity::set_type(const std::string &new_type)
{
	type = new_type;
}



bool entity::test_boundary_collision(SDL_Rect *external_rect)
{
	assert(current_state != NULL);
	assert(external_rect != NULL);
	
	plf::colony<SDL_Rect> collision_blocks;
	SDL_Rect updated_rect;
	
	if (current_state->sprite != NULL && current_state->sprite->has_collision_blocks())
	{
		current_state->sprite->get_collision_blocks(current_state->current_frame_number, collision_blocks);
	}
	else
	{
		collision_blocks = current_state->collision_blocks;
	}
	
	
	for (plf::colony<SDL_Rect>::iterator current_rect = collision_blocks.begin(); current_rect != collision_blocks.end(); ++current_rect)
	{
		updated_rect = *current_rect;
		updated_rect.x = static_cast<int>((updated_rect.x * size) + game_x);
		updated_rect.y = static_cast<int>((updated_rect.y * size) + game_y);
		updated_rect.w = static_cast<int>(static_cast<double>(updated_rect.w) * size);
		updated_rect.h = static_cast<int>(static_cast<double>(updated_rect.h) * size);

		if (SDL_HasIntersection(external_rect, &updated_rect))
		{
			return true;
		}
	}
	
	return false;
}



void entity::get_current_collision_blocks(plf::colony<SDL_Rect> &current_collision_blocks)
{
	assert(current_state != NULL);
	
	// Prefer sprite per-frame collision blocks over state collision blocks
	if (current_state->sprite != NULL && current_state->sprite->has_collision_blocks())
	{
		current_state->sprite->get_collision_blocks(current_state->current_frame_number, current_collision_blocks);
	}
	else if (!(current_state->collision_blocks.empty())) // ie. has blocks
	{
		current_collision_blocks = current_state->collision_blocks;
	}

	for (plf::colony<SDL_Rect>::iterator current_rect = current_collision_blocks.begin(); current_rect != current_collision_blocks.end(); ++current_rect)
	{
		current_rect->x = static_cast<int>((static_cast<double>(current_rect->x) * size) + game_x);
		current_rect->y = static_cast<int>((static_cast<double>(current_rect->y) * size) + game_y);
		current_rect->w = static_cast<int>(static_cast<double>(current_rect->w) * size);
		current_rect->h = static_cast<int>(static_cast<double>(current_rect->h) * size);
	}
}



void entity::set_transparency(const Uint8 new_transparency)
{
	transparency = new_transparency;
}


void entity::set_color_modulation(const Uint8 r, const Uint8 g, const Uint8 b)
{
	if (colormod == NULL)
	{
		colormod = new rgb;
	}
	
	colormod->r = r;
	colormod->g = g;
	colormod->b = b;
}


std::string entity::get_id()
{
	return id;
}


std::string entity::get_type()
{
	return type;
}



std::string entity::get_current_state_id()
{
	return current_state_id;
}



int entity::update(unsigned int delta_time)
{
	if (current_state == NULL) // No states defined
	{
		return -1;
	}

	if (current_state->movement != NULL)
	{
		move(delta_time);
		
		if (layer_quadtree != NULL)
		{
			plf::colony<quadtree *> used_nodes;
			quadtree *parent_node;
			bool detected;

			for (plf::colony<entity_block *>::iterator block_iterator = current_quadtree_blocks.begin(); block_iterator != current_quadtree_blocks.end(); ++block_iterator)
			{
				(*block_iterator)->entity_reference = this; // current solution for account for pointer invalidation by layer's entity vector resizing when entities added/removed - better solution might be to use unique numbers for each entity per layer
				parent_node = (*block_iterator)->parent_node;
				detected = false;
				
				// If we haven't already deleted blocks for this entity from this particular node:
				// (bear in mind the parent nodes may've changed since adding the blocks due to quad subdivision and moving of block references)
				for (plf::colony<quadtree *>::iterator used_nodes_iterator = used_nodes.begin(); used_nodes_iterator != used_nodes.end(); ++used_nodes_iterator)
				{
					if (*used_nodes_iterator == parent_node)
					{
						detected = true;
						break;
					}
				}
				
				if (!detected)
				{
					used_nodes.insert(parent_node);
					parent_node->delete_entity(this);
					// possible future todo HERE: add quadtree node to parent layer iterator of nodes to be potentially consolidated after updates - but would need update/move and new placement of entity in two separate functions - involves iterating over the entire entity vector twice - probably not worth it
					// for moment, consolidating every time (temp):
					parent_node->consolidate_node();
				}
			}
			
			current_quadtree_blocks.clear();
			layer_quadtree->add_entity(this);
		}
	}
	
	const int return_state = current_state->sprite->update_frame(current_state->current_frame_number, current_state->current_sprite_time, static_cast<int>(delta_time), current_state->remainder);

	// TODO: code for optimising when return code has been 21 ie single-frame sprite

	if (current_state->self_destruct_on_sprite_end && return_state == 20) // Means sprite is non-looping and has reached end of it's animation
	{
		return 20; // Return self-destruct code.
	}
	

	// Update sound references:
	for (plf::colony<sound_reference>::iterator reference_iterator = current_state->sound_references.begin(); reference_iterator != current_state->sound_references.end(); ++reference_iterator)
	{
		reference_iterator->update(delta_time, static_cast<int>(game_x), static_cast<int>(game_y));
	}
	
	return 0;
}



int entity::move(const unsigned int delta_time)
{
	assert(current_state->movement != NULL);
	current_state->current_movement_time += delta_time;
	current_state->movement->update(game_x, game_y, delta_time, current_state->current_movement_time, size, flip_horizontal, flip_vertical);

	if (allowed_area != NULL) // Check to see if entity is still within allowed area:
	{
		current_area.x = static_cast<int>(game_x);
		current_area.y = static_cast<int>(game_y);
		
		if (!SDL_HasIntersection(&current_area, allowed_area))
		{
			return 20;  // If return_state is 20 this indicates time to delete the entity ie. Object has moved outside of game world boundaries, or similar.
		}
	}
	
	return 0;
}	





int entity::draw(const double display_x, const double display_y, const Uint8 draw_transparency, rgb *draw_colormod)
{
	if (current_state == NULL)
	{
		return -1;
	}
	
	if (current_state->sprite == NULL) // No sprite in this state
	{
		return 0;
	}
	
	if (draw_transparency == 255 && draw_colormod == NULL) // Optimise for default scenario
	{
		return current_state->sprite->draw_frame(current_state->current_frame_number, static_cast<int>(game_x - display_x), static_cast<int>(game_y - display_y), size, flip_horizontal, flip_vertical, angle, transparency, colormod);
	}

	Uint8 supplied_transparency = transparency;
	
	if (draw_transparency != 255)
	{
		supplied_transparency *= static_cast<Uint8>(static_cast<double>(draw_transparency) / 255);
	}
	

	if (colormod == NULL)
	{
		if (draw_colormod == NULL)
		{
			return current_state->sprite->draw_frame(current_state->current_frame_number, static_cast<int>(game_x - display_x), static_cast<int>(game_y - display_y), size, flip_horizontal, flip_vertical, angle, supplied_transparency, NULL);
		}
		else
		{
			return current_state->sprite->draw_frame(current_state->current_frame_number, static_cast<int>(game_x - display_x), static_cast<int>(game_y - display_y), size, flip_horizontal, flip_vertical, angle, supplied_transparency, draw_colormod);
		}
	}
	else
	{
		if (draw_colormod == NULL)
		{
			return current_state->sprite->draw_frame(current_state->current_frame_number, static_cast<int>(game_x - display_x), static_cast<int>(game_y - display_y), size, flip_horizontal, flip_vertical, angle, supplied_transparency, colormod);
		}
		else
		{
			rgb supplied_colormod;
			supplied_colormod.r = static_cast<Uint8>(static_cast<double>(colormod->r) * (static_cast<double>(draw_colormod->r) / 255));
			supplied_colormod.g = static_cast<Uint8>(static_cast<double>(colormod->g) * (static_cast<double>(draw_colormod->g) / 255));
			supplied_colormod.b = static_cast<Uint8>(static_cast<double>(colormod->b) * (static_cast<double>(draw_colormod->b) / 255));
			return current_state->sprite->draw_frame(current_state->current_frame_number, static_cast<int>(game_x - display_x), static_cast<int>(game_y - display_y), size, flip_horizontal, flip_vertical, angle, supplied_transparency, &supplied_colormod);
		}
	}

	return 0; // This point cannot be reached
}





entity_manager::entity_manager(plf::sound_manager *_sound_manager):
	sound_manager(_sound_manager)
{
	assert(sound_manager != NULL);
}


entity_manager::~entity_manager()
{
	// Destroy Objects:
	for (std::map<std::string, entity *>::iterator entity_iterator = entities.begin(); entity_iterator != entities.end(); ++entity_iterator)
	{
		delete entity_iterator->second;
	}
}


entity * entity_manager::new_entity(const std::string &id)
{
	plf::entity *new_entity = new entity(id, sound_manager);

	std::pair<std::map<std::string, entity *>::iterator, bool> return_value;
	return_value = entities.insert(std::pair<std::string, entity *>(id, new_entity));
	
	plf_assert(return_value.second == true, "plf::engine new_entity error: entity with id '" << id << "' already exists within entities.");
	
	return new_entity;
}



entity * entity_manager::get_entity(const std::string &id)
{
	std::map<std::string, entity *>::iterator entity_iterator = entities.find (id);
	
	// if entity doesn't exist, return null, otherwise:
	if (entity_iterator == entities.end())
	{
		return NULL;
	}
	
	return entity_iterator->second;
}



int entity_manager::remove_entity(const std::string &id)
{
	std::map<std::string, entity *>::iterator entity_iterator = entities.find (id);
	
	// if entity doesn't exist, return -1:
	if (entity_iterator == entities.end())
	{
		return -1;
	}
	
	delete entity_iterator->second;
	entities.erase(entity_iterator);
	return 0;
}

}