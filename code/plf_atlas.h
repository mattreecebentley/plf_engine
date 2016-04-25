#ifndef PLF_ATLAS_H
#define PLF_ATLAS_H

#include <vector>

#include <SDL2/SDL.h>

#include "plf_renderer.h"


// A class detailing a segment of the texture atlas + subsegments - created recursively:

namespace plf
{


class atlas_node
{
private:
	SDL_Rect *image_rect; // image_rect currently serves as both an indicator as to whether a node has an image in it, and a time-saving measure in terms of storing a permanent SDL_Rect to return. It is kinda redundant though, as it's information is already stored in the x,y etc coordinates below. Could be replaced with a bool
	atlas_node *parent_node, *split_a, *split_b;
	unsigned int x, y, width, height;

	friend class atlas;
	friend class atlas_manager;
	friend class texture;
	friend class multitexture;

	atlas_node(const unsigned int node_x, const unsigned int node_y, const unsigned int node_width, const unsigned int node_height, atlas_node *parent);
	~atlas_node();

	atlas_node * add(const unsigned int image_width, const unsigned int image_height);
	SDL_Rect * get_node_coordinates();
	SDL_Rect * get_image_coordinates(); // node can be slightly larger than image (thereoretically - would need to implement pixel buffering in future for this to be true).
	bool is_empty() {return image_rect == NULL;};
	void consolidate_empty_children();
	bool node_and_child_nodes_are_empty();
	
};




class atlas
{
private:
	SDL_Texture *atlas_texture;
	plf::renderer *renderer;
	atlas_node *prime_node; // ie. top-level node of entire atlas - contains entire atlas within it
public:
	atlas(plf::renderer *p_renderer, const unsigned int atlas_width, const unsigned int atlas_height);
	~atlas();
	atlas_node * add_surface(SDL_Surface *external_surface);
	void remove_surface(atlas_node *node);

	SDL_Texture * get_texture();
};




class atlas_manager
{
private:
	std::vector<atlas *> atlases;
	plf::renderer *renderer;
	int maximum_width, maximum_height;

public:
	atlas_manager(plf::renderer *_renderer);
	~atlas_manager();
	
	std::pair<atlas *, atlas_node *> add_surface(SDL_Surface *new_surface);

	// utility function in case you want to see what the atlas itself looks like, or whatever:
	SDL_Texture * get_atlas_texture(const unsigned int atlas_number); // first number is 1, not 0.
	void get_maximum_texture_size(int &width, int &height);
	
	// Developer function - for testing purposes:
	inline unsigned int get_number_of_atlases() { return static_cast<unsigned int>(atlases.size()); };
};


}

#endif // PLF_ATLAS_H
