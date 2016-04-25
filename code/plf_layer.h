#ifndef PLF_LAYER_H
#define PLF_LAYER_H

#include <vector>
#include <string>

#include <SDL2/SDL.h>

#include "plf_entity.h"
#include "plf_quadtree.h"
#include "plf_colony.h"



namespace plf
{

class layer
{
private:
	struct background
	{
		plf::sprite *sprite;
		double resize;
		unsigned int sprite_time;
		int x, y;
	};

	plf::colony <background> backgrounds;
	plf::colony <entity> entities[10];
	std::string id;
	plf::quadtree *quadtree;
	SDL_Rect boundaries;
	
	rgb *layer_colormod;
	double move_relative_xy; // Variable for parallax layer movement (both horizontal and vertical movement),
							// 0 = does not move
							// 0.25 = moves a quarter as much as player layer
							// 1 = moves at same rate as player layer (same layer-ish backgrounds)
							// 1.5 = moves at 150% rate of player layer (in front of player layer, for example)
							// -1 = moves at the same rate as player, but backwards
	unsigned int total_number_of_entities;
	Uint8 layer_transparency;
public:
	layer(const std::string &layer_id, const double relative_movement_rate, const int x, const int y, const unsigned int width, const unsigned int height);
	~layer();

	void add_background(sprite *sprite, const int x, const int y, double size);
	entity * spawn_entity(const std::string &new_id, entity *entity, const int entity_x, const int entity_y, const unsigned int sprite_time_displacement = 0, const unsigned int movement_time_displacement = 0, const double size = 1, const unsigned int z_index = 0);
	int remove_entities(const std::string &id);
	std::vector <entity *> get_entities(const std::string &id);
	void draw(const unsigned int delta_time, const int display_x, const int display_y); // Display_xy are the upper-left coordinates of the games current view.
	int update(const unsigned int delta_time);
	void set_transparency(const Uint8 new_transparency); // Of all backgrounds and entities on layer
	void set_color_modulation(const Uint8 r, const Uint8 g, const Uint8 b); // ditto
	inline std::string get_id() { return id; };
	inline void get_collisions(std::vector< std::pair<entity *, entity *> > &collision_pairs) { quadtree->get_collisions(collision_pairs); };

	// This is primarily for developer bugshooting:
   void show_quadtree(plf::renderer *plf_renderer, const int display_x, const int display_y, Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
};




class layer_manager
{
private:
	struct layer_reference
	{
		plf::layer *layer;
		int z_index;
	};

	// All layers used in game:
	std::vector<layer_reference> layers;

public:
	layer_manager();
	~layer_manager();
	layer * new_layer(const std::string &id, const int z_index, const double relative_movement, const int x, const int y, const unsigned int width, const unsigned int height);
	layer * get_layer(const std::string &id);
	layer * get_layer(const int z_index);
	int assign_layer(layer *layer_to_add, const int z_index);
	int remove_layer(const std::string &id);
	int remove_layer(const int z_index);
	void update_layers(const unsigned int delta_time);
	void draw_layers(const unsigned int delta_time, const int display_x, const int display_y);
	void get_all_collisions(std::vector< std::pair<entity *, entity *> > &collision_pairs);
};



}

#endif // PLF_LAYER_H
