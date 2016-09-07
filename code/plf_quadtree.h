#ifndef PLF_QUADTREE_H
#define PLF_QUADTREE_H

#include <vector>

#include <SDL2/SDL.h>

#include "plf_entity.h"
#include "plf_colony.h"


namespace plf
{

class quadtree; // forward declaration for entity_block


// Because we're now using colonies and not vectors for holding blocks, you could actually get rid of this struct entirely and simply have a colony of pointers to the original entity blocks within the entities themselves. Still, a lot of work. With vectors had to have a complicated structure to deal with vector iterator/pointer invalidation. Unnecessary with colony.
struct entity_block
{
	quadtree *parent_node;
	entity *entity_reference;
	SDL_Rect rect;
	int right, bottom;

	inline bool contains(int x, int y) { return (x >= rect.x) && (x <= right) && (y >= rect.y) && (y <= bottom); };

	inline bool test_boundary_collision(SDL_Rect *external_rect)
	{
		if (SDL_HasIntersection(external_rect, &rect) == SDL_TRUE)
		{
			return true;
		}

		return false;
	};
};




class quadtree
{
private:
	enum node_location
	{
		NW = 0,
		NE,
		SW,
		SE,
		NODE_COUNT
	};

	enum quad_type
	{
		UNSPLIT = 0,
		SPLIT,
		CANNOT_SPLIT // ie. is lowest level of node, based on minimum node width and height
	};

	plf::colony<entity_block *> blocks;
	plf::colony<entity_block *> large_blocks;

	quadtree *nodes[4];
	quadtree *parent_node;

	quad_type split_status;

	int left, right, top, bottom;
	int middle_x, middle_y; // mid-point x and y of current node. Used a lot
	int half_width, half_height;
	unsigned int minimum_width, minimum_height; // The smallest possible size a node can be
	unsigned int entity_limit; // Maximum number of (small) entities per node before splitting occurs

	// This function recursively gathers sub-node's blocks (and sub-collisions) before comparing them to the current node's blocks. Hence it requires the entity_block vector supplied to it
	void get_collisions_and_blocks(std::vector< std::pair<entity *, entity *> > &collision_pairs, std::vector<entity_block *> &block_collection);
	void add_block(entity_block *new_block);
	int move_block_to_subnode(entity_block *new_block);
	void check_children_then_consolidate(quadtree *child_node); // Find out if child nodes are empty, if so delete them and this.

public:
	quadtree(quadtree *_parent_node, const int _left, const int _right, const int _top, const int _bottom, const unsigned int _minimum_width, const unsigned int _minimum_height, const unsigned int _entity_limit = 3);
	~quadtree();

	void clear();
	bool is_empty();

	void add_entity(entity *new_entity);
	void delete_entity(entity *entity); // Delete any blocks from this node associated with this entity
	void consolidate_node(); // After deleting blocks, test this node to see whether it's empty and whether it and it's parents can be consolidated

	void get_collisions(std::vector< std::pair<entity *, entity *> > &collision_pairs);

  	// For developer tests: displays the quadtree onscreen, given an appropriate SDL_Renderer pointer, with the rgb color assigned (black by default). Have not stored the plf_renderer point in the quadtree as that would be a waste of resources for non-debug builds.
	void display(SDL_Renderer *renderer, const int display_x, const int display_y, Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);

	// For internal testing purposes only, do not document:
	void get_blocks_at(const int x, const int y, plf::colony<entity_block *> &block_collection); // Returns all blocks at given coordinates
	unsigned int get_number_of_blocks_at(const int x, const int y); // Returns the number of quadtree blocks at the coordinates. Testing function, unused, do not document.
	int delete_blocks_at(const int x, const int y); // Delete any quadtree blocks which contain the following cooordinates. This was a testing function and is unused. Do not document.
};


}
#endif // quadtree_H
