#include <cmath> // For abs
#include <cassert>
#include <vector>

#include <SDL2/SDL.h>

#include "plf_quadtree.h"
#include "plf_entity.h"
#include "plf_colony.h"


namespace plf
{

quadtree::quadtree(quadtree *_parent_node, const int _left, const int _right, const int _top, const int _bottom, const unsigned int _minimum_width, const unsigned int _minimum_height, const unsigned int _entity_limit) : 
	parent_node(_parent_node),
	split_status(UNSPLIT), 
	left(_left),
	right(_right),
	top(_top),
	bottom(_bottom),
	half_width(std::abs(right - left) / 2),
	half_height(std::abs(bottom - top) / 2),
	minimum_width(_minimum_width),
	minimum_height(_minimum_height),
	entity_limit(_entity_limit)
{
	for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
	{
		nodes[node_number] = NULL;
	}
	
	if ((static_cast<int>(minimum_width) > half_width) || (static_cast<int>(minimum_height) > half_height))
	{
		split_status = CANNOT_SPLIT; // ie. lowest level of node based on minimum node width and height
	}
}



quadtree::~quadtree()
{
	clear();
}



int quadtree::move_block_to_subnode(entity_block *block)
{
	if (block->rect.x <= middle_x)
 	{
 		if (block->right <= middle_x) // Parent else if follow-on depends on parent if block's negated precondition but not this one
 		{
 			if (block->rect.y <= middle_y)
 			{
 				if (block->bottom <= middle_y)
 				{
 					nodes[NW]->add_block(block);
 					return 0;
 				}
 			}
 			else // if (block->bottom > middle_y) - implied by if - bottom will not be smaller than y
 			{
 				nodes[SW]->add_block(block);
 				return 0;
 			}
 		}
 	}
 	else // if (block->right > middle_x) - implied by first if calculation
 	{
 		if (block->rect.y <= middle_y)
 		{
 			if (block->bottom <= middle_y)
 			{
 				nodes[NE]->add_block(block);
 				return 0;
 			}
 		}
 		else // if (block->bottom > middle_y) - also implied by if - bottom will not be smaller than y
 		{
 			nodes[SE]->add_block(block);
 			return 0;
 		}
 	}
 
 	// entity is not moved, does not fit in any subnode
 	return -1;
}



void quadtree::add_entity(entity *entity)
{
	plf::colony<SDL_Rect> blocks;
	entity->get_current_collision_blocks(blocks);
	entity_block *block_to_add;

	for (plf::colony<SDL_Rect>::iterator block_iterator = blocks.begin(); block_iterator != blocks.end(); ++block_iterator)
	{
		block_to_add = new entity_block;
		block_to_add->entity_reference = entity;
		block_to_add->rect = *block_iterator;
		block_to_add->right = block_to_add->rect.x + block_to_add->rect.w;
		block_to_add->bottom = block_to_add->rect.y + block_to_add->rect.h;
		
		add_block(block_to_add);
		entity->add_quadtree_block(block_to_add);
	}
}



void quadtree::add_block(entity_block *block)
{
	if (split_status == UNSPLIT)
	{
		if (block->rect.w < half_width && block->rect.h < half_height) // if entity will potentially fit within a subnode
		{
			if (blocks.size() >= entity_limit)
			{
				middle_x = left + half_width;
				middle_y = top + half_height;
							
				nodes[NW] = new quadtree(this, left, middle_x, top, middle_y, minimum_width, minimum_height, entity_limit);
				nodes[NE] = new quadtree(this, middle_x, right, top, middle_y, minimum_width, minimum_height, entity_limit);
				nodes[SW] = new quadtree(this, left, middle_x, middle_y, bottom, minimum_width, minimum_height, entity_limit);
				nodes[SE] = new quadtree(this, middle_x, right, middle_y, bottom, minimum_width, minimum_height, entity_limit);

				for (plf::colony<entity_block *>::iterator block_iterator = blocks.begin(); block_iterator != blocks.end();)
				{
					if (move_block_to_subnode(*block_iterator) == 0)
					{
						block_iterator = blocks.erase(block_iterator);
					}
					else
					{
						++block_iterator;
					}
				}
				
				split_status = SPLIT;

				if (move_block_to_subnode(block) == 0) // Moved to subnode
				{
					return;
				}
			}

			block->parent_node = this;
			blocks.insert(block); // Default action
			return;
		}
	}
	else if (split_status == SPLIT && block->rect.w < half_width && block->rect.h < half_height)
	{
		if (move_block_to_subnode(block) == 0) // Moved to subnode
		{
			return;
		}

		block->parent_node = this;
		blocks.insert(block);
		return;
	}
	
	block->parent_node = this;
	blocks.insert(block); // ie. Default action when entity is large or split_status == CANNOT SPLIT
}



void quadtree::delete_entity(entity *entity)
{
	for (plf::colony<entity_block *>::iterator current_block = blocks.begin(); current_block != blocks.end();)
	{
		if ((*current_block)->entity_reference == entity)
		{
			delete *current_block;
			current_block = blocks.erase(current_block);
		}
		else
		{
			++current_block;
		}
	}


	for (plf::colony<entity_block *>::iterator current_block = large_blocks.begin(); current_block != large_blocks.end();)
	{
		if ((*current_block)->entity_reference == entity)
		{
			delete *current_block;
			current_block = large_blocks.erase(current_block);
		}
		else
		{
			++current_block;
		}
	}
}



// to be used after a series of deletions of blocks in node by an entity
void quadtree::consolidate_node() 
{
	// If this node or it's subnodes aren't empty:
	if (split_status == SPLIT || !(blocks.empty()) || !(large_blocks.empty()) || parent_node == NULL)
	{
		return;
	}
	
	parent_node->check_children_then_consolidate(this);
}



void quadtree::check_children_then_consolidate(quadtree *child_node) 
{
	assert(split_status == SPLIT); // If not the case, something is very, very wrong. This function should ONLY be called by a child node!
	
	// Check emptiness status of child nodes, avoiding check of the calling child node (assumed to be already empty):
	unsigned int empty_node_count = 1;

	for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
	{
		if (nodes[node_number] != child_node && nodes[node_number]->is_empty())
		{
			empty_node_count++;
		}
	}

	// If not all child nodes are empty or blocks size is over the entity limit, cancel operation:
	if (empty_node_count != NODE_COUNT || blocks.size() >= entity_limit)
	{
		return;
	}
	
	for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
	{
		delete nodes[node_number];
		nodes[node_number] = NULL;
	}
	
	split_status = UNSPLIT;

	// If node itself is also empty, recurse consolidation up the tree:
	if (blocks.empty() && large_blocks.empty() && parent_node != NULL)
	{
		parent_node->check_children_then_consolidate(this);
	}
}

	

void quadtree::clear()
{
	for (plf::colony<entity_block *>::iterator current_block = blocks.begin(); current_block != blocks.end(); ++current_block)
	{
		delete *current_block;
	}

	for (plf::colony<entity_block *>::iterator current_block = large_blocks.begin(); current_block != large_blocks.end(); ++current_block)
	{
		delete *current_block;
	}

	blocks.clear();
	large_blocks.clear();
	
	if (split_status == SPLIT)
	{
		for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
		{
			delete nodes[node_number];
			nodes[node_number] = NULL;
		}
		
		split_status = UNSPLIT;
	}
}



bool quadtree::is_empty()
{
	if (!(blocks.empty()) || !(large_blocks.empty()))
	{
		return false;
	}
	
	if (split_status != SPLIT)
	{
		return true;
	}
	
	for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
	{
		if (!(nodes[node_number]->is_empty()))
		{
			return false;
		}
	}
	
	return true;
}



void quadtree::get_blocks_at(const int x, const int y, plf::colony<entity_block *> &block_collection)
{
	if (!blocks.empty())
	{
		block_collection.insert(blocks.begin(), blocks.end());
	}

	if (!large_blocks.empty())
	{
		block_collection.insert(large_blocks.begin(), large_blocks.end());
	}

	if (split_status != SPLIT)
	{
		return;
	}
	
	int selected_node;
	if (x > middle_y)
	{
		selected_node = NE;
	}
	else
	{
		selected_node = NW;
	}
	
	if (y > middle_y)
	{
		selected_node += 2; // Changes it to south
	}

	nodes[selected_node]->get_blocks_at(x, y, block_collection);
}



unsigned int quadtree::get_number_of_blocks_at(const int x, const int y)
{
	if (split_status != SPLIT)
	{
		return static_cast<unsigned int>(blocks.size() + large_blocks.size());
	}
	
	int selected_node = NW;

	if (x > middle_x)
	{
		selected_node = NE;
	}
	
	if (y > middle_y)
	{
		selected_node += 2; // Changes it to south
	}

	return static_cast<unsigned int>(blocks.size() + large_blocks.size() + nodes[selected_node]->get_number_of_blocks_at(x, y));
}



int quadtree::delete_blocks_at(const int x, const int y)
{
	for (plf::colony<entity_block *>::iterator current_block = blocks.begin(); current_block != blocks.end();)
	{
		if ((*current_block)->contains(x, y))
		{
			delete *current_block;
			current_block = blocks.erase(current_block);
		}
		else
		{
			++current_block;
		}
	}

	for (plf::colony<entity_block *>::iterator current_block = large_blocks.begin(); current_block != large_blocks.end();)
	{
		if ((*current_block)->contains(x, y))
		{
			delete *current_block;
			current_block = large_blocks.erase(current_block);
		}
		else
		{
			++current_block;
		}
	}

	if (split_status != SPLIT)
	{
		return 0;
	}
	

	int selected_node = NW;

	if (x > middle_x)
	{
		selected_node = NE;
	}
	
	if (y > middle_y)
	{
		selected_node += 2; // Changes it to south
	}

	nodes[selected_node]->delete_blocks_at(x, y);
	

	unsigned int empty_node_count = 0;

	for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
	{
		if (nodes[node_number]->is_empty())
		{
			empty_node_count++;
		}
	}

	
	if (empty_node_count == NODE_COUNT)
	{
		for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
		{
			delete nodes[node_number];
		}
		
		split_status = UNSPLIT;
	}

	return 0;
}



void quadtree::get_collisions_and_blocks(std::vector< std::pair<entity *, entity *> > &collision_pairs, std::vector<entity_block *> &block_collection)
{
	if (!blocks.empty() || !large_blocks.empty())
	{
		entity *current_block_entity, *comparison_block_entity;
		SDL_Rect *current_block_rect, *comparison_block_rect;
		std::vector<entity_block *> all_blocks;

		all_blocks.insert(all_blocks.end(), large_blocks.begin(), large_blocks.end());
		all_blocks.insert(all_blocks.end(), blocks.begin(), blocks.end());
		
		if (all_blocks.size() != 1)
		{
			// Check collisions at current node level:
			
			std::vector<entity_block *>::iterator end_iterator = all_blocks.end();
			--end_iterator;
			
			for (std::vector<entity_block *>::iterator block_iterator = all_blocks.begin(); block_iterator != end_iterator; ++block_iterator)
			{
				current_block_entity = (*block_iterator)->entity_reference;
				current_block_rect = &((*block_iterator)->rect);
				
				std::vector<entity_block *>::iterator comparison_iterator = block_iterator;
				++comparison_iterator;
				
				for (; comparison_iterator != all_blocks.end(); ++comparison_iterator)
				{
					comparison_block_entity = (*block_iterator)->entity_reference;
					comparison_block_rect = &((*comparison_iterator)->rect);
					
					// rule out collision between two blocks from the same entity, then test for collision:
					if ((comparison_block_entity != current_block_entity) && SDL_HasIntersection(current_block_rect, comparison_block_rect)) // == true
					{
						collision_pairs.push_back(std::make_pair(current_block_entity, comparison_block_entity));
					}
				}
			}
		}
		
		if (split_status == SPLIT)
		{
			// Retrieve subnode blocks and let subnodes test for collisions:
			for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
			{
				nodes[node_number]->get_collisions_and_blocks(collision_pairs, block_collection);
			}

			// Test retrieved list against this node level's blocks:
			for (std::vector<entity_block *>::iterator block_iterator = all_blocks.begin(); block_iterator != all_blocks.end(); ++block_iterator)
			{
				current_block_entity = (*block_iterator)->entity_reference;
				current_block_rect = &((*block_iterator)->rect);
				
				for (std::vector<entity_block *>::iterator comparison_iterator = block_collection.begin(); comparison_iterator != block_collection.end(); ++comparison_iterator)
				{
					// rule out collision between two blocks from the same entity, then test for collision:
					if (((*comparison_iterator)->entity_reference != current_block_entity) && (*comparison_iterator)->test_boundary_collision(current_block_rect)) // == true
					{
						collision_pairs.push_back(std::make_pair((*block_iterator)->entity_reference, (*comparison_iterator)->entity_reference));
					}
				}
			}
		}
		
		block_collection.insert(block_collection.end(), all_blocks.begin(), all_blocks.end());
		
		return;
	}
	
	if (split_status == SPLIT)
	{
		// Retrieve subnode blocks and let subnodes test for collisions:
		for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
		{
			nodes[node_number]->get_collisions_and_blocks(collision_pairs, block_collection);
		}
	}
}



void quadtree::get_collisions(std::vector< std::pair<entity *, entity *> > &collision_pairs)
{
	std::vector<entity_block *> block_collection;
	get_collisions_and_blocks(collision_pairs, block_collection);
}



void quadtree::display(SDL_Renderer *renderer, const int displacement_x, const int displacement_y, Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Rect rect;

	if (r != 0 || g != 0 || b != 0)
	{
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	}

	for (plf::colony<entity_block *>::iterator current_block = blocks.begin(); current_block != blocks.end(); ++current_block)
	{
		rect = (*current_block)->rect;
		rect.x -= displacement_x;
		rect.y -= displacement_y;
		SDL_RenderDrawRect(renderer, &rect);
	}

	for (plf::colony<entity_block *>::iterator current_block = large_blocks.begin(); current_block != large_blocks.end(); ++current_block)
	{
		rect = (*current_block)->rect;
		rect.x -= displacement_x;
		rect.y -= displacement_y;
		SDL_RenderDrawRect(renderer, &((*current_block)->rect));
	}

	if (split_status == SPLIT)
	{
		for (unsigned int node_number = 0; node_number != NODE_COUNT; ++node_number)
		{
			nodes[node_number]->display(renderer, displacement_x, displacement_y, r, g, b);
		}
	}

	rect.x = left - displacement_x;
	rect.y = top - displacement_y;
	rect.w = right - left;
	rect.h = bottom - top;
	SDL_RenderDrawRect(renderer, &rect);
}

}
