#include <vector>
#include <cassert>

#include <SDL2/SDL.h>

#include "plf_entity.h"
#include "plf_layer.h"
#include "plf_utility.h"
#include "plf_quadtree.h"
#include "plf_colony.h"


namespace plf
{


	layer::layer(const std::string &layer_id, const double relative_movement_rate, const int x, const int y, const unsigned int width, const unsigned int height):
		id(layer_id),
		layer_colormod(NULL),
		move_relative_xy(relative_movement_rate),
		total_number_of_entities(0),
		layer_transparency(255)
	{
		assert(id != "");
		boundaries.x = x;
		boundaries.y = y;
		boundaries.w = static_cast<int>(width);
		boundaries.h = static_cast<int>(height);
		
		unsigned int largest_dimension = width; // want square nodes
		if (width < height) largest_dimension = height;
		
		quadtree = new plf::quadtree(NULL, x, x + largest_dimension, y, y + largest_dimension, 50, 50);
	}
	
	
	
	layer::~layer()
	{
		// Sprites are cleaned up separately, so no deallocation necessary. Backgrounds and entities are statically allocated, no dynamic garbage collection required.
		delete layer_colormod;
		delete quadtree;
	}
	
	
	
	void layer::add_background(sprite *sprite, const int x, const int y, double size)
	{
		assert(sprite->has_frames()); // ie. has content
		assert(size > 0 && size <= 1000); // Sanity-check 
	
	//	backgrounds.push_back(background());
	//	background *background_pointer = &(backgrounds.back());
		// changing to colony
		
		background *background_pointer = &*(backgrounds.insert(background()));
	
		background_pointer->sprite = sprite;
		background_pointer->x = x;
		background_pointer->y = y;
		background_pointer->resize = size;
	}
	
	
	
	entity * layer::spawn_entity(const std::string &new_id, entity *entity, const int entity_x, const int entity_y, const unsigned int sprite_time_offset, const unsigned int movement_time_offset, const double size, const unsigned int z_index)
	{
		assert(entity != NULL);
		assert(z_index < 30); // Z-index is 0-29
		assert(size > 0 && size <= 1000); // sanity-check
	
		plf::entity *copied_entity = &*(entities[z_index].insert(*entity));
		copied_entity->set_size(size);
		copied_entity->set_location(static_cast<double>(entity_x), static_cast<double>(entity_y));
		copied_entity->set_id(new_id);
		copied_entity->set_sprite_time_offset(sprite_time_offset);
		copied_entity->set_movement_time_offset(movement_time_offset);
		copied_entity->set_quadtree(quadtree);
	
		quadtree->add_entity(copied_entity);
		
		return copied_entity;
	}
	
	
	
	void layer::set_transparency(const Uint8 new_transparency)
	{
		layer_transparency = new_transparency;
	}
	
	
	
	void layer::set_color_modulation(const Uint8 r, const Uint8 g, const Uint8 b)
	{
		if (layer_colormod == NULL)
		{
			layer_colormod = new rgb;
		}
		
		layer_colormod->r = r;
		layer_colormod->g = g;
		layer_colormod->b = b;
	}
	
	
	
	void layer::draw(const unsigned int delta_time, const int display_x, const int display_y)
	{
		// Display background images:
		double adjusted_x = (static_cast<double>(display_x) * move_relative_xy);
		double adjusted_y = (static_cast<double>(display_y) * move_relative_xy);
	
		if (!(backgrounds.empty()))
		{
			for(plf::colony<background>::iterator background_iterator = backgrounds.begin(); background_iterator != backgrounds.end(); ++background_iterator)
			{
				background_iterator->sprite->draw(background_iterator->sprite_time, delta_time, background_iterator->x - static_cast<int>(adjusted_x), background_iterator->y - static_cast<int>(adjusted_y), background_iterator->resize, false, false, 0, layer_transparency, layer_colormod);
			}
		}
	
		for (unsigned int z_index = 0; z_index != 10; ++z_index)
		{
			if (!(entities[z_index].empty()))
			{
				for(plf::colony<entity>::iterator entity_iterator = entities[z_index].begin(); entity_iterator != entities[z_index].end(); ++entity_iterator)	
				{
					// Display background:
					entity_iterator->draw(adjusted_x, adjusted_y, layer_transparency, layer_colormod);
				}
			}
		}
	}
	
	
		
	int layer::update(const unsigned int delta_time)
	{
		int return_state;
		unsigned int vector_position;
		unsigned int vector_size;
		unsigned int empty_entity_z_indexes = 0;
		plf::colony<entity>::iterator entity_iterator;
		
		
		for (unsigned int z_index = 0; z_index != 10; ++z_index)
		{
			vector_position = 0;
			vector_size = static_cast<unsigned int>(entities[z_index].size());
			
			if (vector_size != 0)
			{
				entity_iterator = entities[z_index].begin(); 
	
				do
				{
					return_state = entity_iterator->update(delta_time);
					
					if (return_state != 20)
					{
						++entity_iterator;
						++vector_position;
					}
					else // ie. Update function indicates that entity has moved outside of world boundaries or similar 'end state'/self-destruct scenario
					{
						entity_iterator = entities[z_index].erase(entity_iterator);
						--vector_size;
						
						if (vector_size == 0)
						{
							++empty_entity_z_indexes;
							vector_position = vector_size;
						}
						else if (entity_iterator == entities[z_index].end())
						{
							vector_position = vector_size;
						}
					}
				} while (vector_position < vector_size);
			}
			else
			{
				++empty_entity_z_indexes;
			}
		}
		
		if (empty_entity_z_indexes == 10)
		{
			return 20; // Indicates layer can be removed, no entities left
		}
		
		return 0;
	}
	
	
	
	std::vector<entity *> layer::get_entities(const std::string &id)
	{
		std::vector<entity *> id_matched_entities;
		
		for (unsigned int z_index = 0; z_index != 10; ++z_index)
		{
			if(!entities[z_index].empty())
			{
				for(plf::colony<entity>::iterator entity_iterator = entities[z_index].begin(); entity_iterator != entities[z_index].end(); ++entity_iterator)
				{
					if (entity_iterator->get_id() == id)
					{
						id_matched_entities.push_back(&*entity_iterator);
					}
				}
			}
		}
		
		return id_matched_entities;
	}
	
	
	
	int layer::remove_entities(const std::string &id)
	{
		int number_of_erased_entities = 0;
		
		for (unsigned int z_index = 0; z_index != 10; ++z_index)
		{
			if(!entities[z_index].empty())
			{
				for(plf::colony<entity>::iterator entity_iterator = entities[z_index].begin(); entity_iterator != entities[z_index].end(); ++entity_iterator)
				{
					if (entity_iterator->get_id() == id)
					{
						entity_iterator = entities[z_index].erase(entity_iterator);
						++number_of_erased_entities;
					}
				}
			}
		}
		
		return number_of_erased_entities;
	}
	
	
	
	
	void layer::show_quadtree(plf::renderer *renderer, const int display_x, const int display_y, Uint8 r, Uint8 g, Uint8 b)
	{
		quadtree->display(renderer->get(), static_cast<int>(display_x * move_relative_xy), static_cast<int>(display_y * move_relative_xy), r, g, b);
	}
	
	
	layer_manager::layer_manager()
	{
	}
	
	
	
	layer_manager::~layer_manager()
	{
		// Destroy Layers:
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			delete layer_iterator->layer;
		}
	}
	
	
	
	layer * layer_manager::new_layer(const std::string &id, const int z_index, const double relative_movement, const int x, const int y, const unsigned int width, const unsigned int height)
	{
		plf_assert(get_layer(z_index) == NULL, "plf::engine new_layer error: layer with z_index '" << z_index << "' already exists.");
		plf_assert(get_layer(id) == NULL, "plf::engine new_layer error: layer with id '" << id << "' already exists.");
		
		layer *new_layer = new layer(id, relative_movement, x, y, width, height);
		layer_reference new_reference;
		new_reference.z_index = z_index;
		new_reference.layer = new_layer;
		
		for (std::vector<layer_reference>::reverse_iterator layer_iterator = layers.rbegin(); layer_iterator != layers.rend(); ++layer_iterator)
		{
			if (layer_iterator->z_index < z_index)
			{
				layers.insert(layer_iterator.base(), new_reference);
				return new_layer;
			}
		}
		
		layers.insert(layers.begin(), new_reference);
		return new_layer;
	}
	
	
	
	int layer_manager::assign_layer(layer *layer_to_add, const int z_index)
	{
		assert(layer_to_add != NULL);
		assert(get_layer(z_index) == NULL);
		plf_assert(get_layer(layer_to_add->get_id()) == NULL, "plf::engine assign_layer error: layer with id '" << layer_to_add->get_id() << "' already exists.");
	
		layer_reference new_reference;
		new_reference.z_index = z_index;
		new_reference.layer = layer_to_add;
	
		for (std::vector<layer_reference>::reverse_iterator layer_iterator = layers.rbegin(); layer_iterator != layers.rend(); ++layer_iterator)
		{
			if (layer_iterator->z_index < z_index)
			{
				layers.insert(layer_iterator.base(), new_reference);
				return 0;
			}
		}
		
		layers.insert(layers.begin(), new_reference);
		return 0;
	}
	
	
	
	layer * layer_manager::get_layer(const std::string &id)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			if (layer_iterator->layer->get_id() == id)
			{
				return layer_iterator->layer;
			}
		}
	
		// This is also a utility function to check for layer existence, logging and fail not required.
		return NULL;
	}
	
	
	
	layer * layer_manager::get_layer(const int z_index)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			if (layer_iterator->z_index == z_index)
			{
				return layer_iterator->layer;
			}
		}
	
		// This is also a utility function to check for layer existence, logging and fail not required.
		return NULL;
	}
	
	
	
	int layer_manager::remove_layer(const std::string &id)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			if (layer_iterator->layer->get_id() == id)
			{
				delete layer_iterator->layer;
				layer_iterator->layer = NULL;
				layers.erase(layer_iterator);
				return 0;
			}
		}
		
		std::clog << "plf::engine remove_layer error: layer with id '" << id << "' not found." << std::endl;
		return -1;
	}
	
	
	
	int layer_manager::remove_layer(const int z_index)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			if (layer_iterator->z_index == z_index)
			{
				delete layer_iterator->layer;
				layer_iterator->layer = NULL;
				layers.erase(layer_iterator);
				return 0;
			}
		}
		
		std::clog << "plf::engine remove_layer error: layer with z_index '" << z_index << "' not found." << std::endl;
		return -1;
	}
	
	
	
	void layer_manager::update_layers(const unsigned int delta_time)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			layer_iterator->layer->update(delta_time);
		}
	}
	
	
	void layer_manager::draw_layers(const unsigned int delta_time, const int display_x, const int display_y)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			layer_iterator->layer->draw(delta_time, display_x, display_y);
		}
	}
	
	
	
	void layer_manager::get_all_collisions(std::vector< std::pair<entity *, entity *> > &collision_pairs)
	{
		for (std::vector<layer_reference>::iterator layer_iterator = layers.begin(); layer_iterator != layers.end(); ++layer_iterator)
		{
			layer_iterator->layer->get_collisions(collision_pairs); // Adds to vector
		}	
	}

}