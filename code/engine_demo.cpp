#include <fstream> // log redirection
#include <cmath> // bird movement
#include <vector> // collision storage

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "plf_engine.h"



class bird_movement : public plf::movement
{
private:
	unsigned int timer;
public:
	bird_movement(): timer(0) {};
	bird_movement * clone() { return new bird_movement(); };
	
	void update(double &current_x, double &current_y, const unsigned int delta_time, const unsigned int movement_time, const double resize_movement = 1, const bool horizontal_flip = false, const bool vertical_flip = false)
	{
		add_impulse_velocity(((double)delta_time / 10.0) * resize_movement, resize_movement * (double)delta_time * 0.3 * sin(((double)movement_time / 10.0) * ((2.0 * 3.1416) / 180.0)), 1);
		default_update(current_x, current_y, delta_time, movement_time, resize_movement);
	}
};




int main( int argc, char* args[] )
{
	freopen("plf.log","w",stderr); //redirect std::clog and std::cerr to file

	plf::engine *engine = new plf::engine;

	plf::log logmain("main_log.txt"); // Create a log specifically for this main, separate from the plf log
	
	// Print all possible display modes to log:
	std::vector<SDL_DisplayMode> display_modes;
	engine->get_all_display_modes(display_modes);

	logmain << "Available display modes:" << std::endl;

	for (std::vector<SDL_DisplayMode>::iterator current_mode = display_modes.begin(); current_mode != display_modes.end(); ++current_mode)
	{
		logmain << current_mode->w << ", " << current_mode->h << ", " << current_mode->refresh_rate << "hz" << std::endl; 
	}
	
	
	// Initialize display etc:
	engine->initialize("plf test", 1024, 768, 1024, 768, plf::WINDOWED, plf::VSYNC_OFF);
	
	// Blank the screen:
	engine->renderer->clear_screen();

	SDL_Event event;



	// 1 - spawn tons of birds onto three layers and display quadtrees, with a large image in background (will be multi-textured automatically due to size).
	// Collect collisions between birds each frame and explode one of each of the two birds which collide.
	// Stagger animation timings so birds don't look like they're all moving/exploding in sync.

	engine->sound->set_audibility_radius(200); // Set pixel range outside of which we don't hear sounds from a given entity
	engine->sound->set_stereo_radius(200); // Set pixel limit within which changes to the stereo field are made (left and right)

	// Setup randomised bird sounds with each chance of each sound:
	engine->sound->add_sound("caw1", "../caw2.wav");
	engine->sound->add_sound("caw2", "../caw1.wav");
	engine->sound->add_sound("caw3", "../caw3.wav");

	plf::random_sound *randomised = engine->sound->add_random_sound("random_caws");
	randomised->add_sound("caw1", 10);
	randomised->add_sound("caw2", 10);
	randomised->add_sound("caw3", 10);
	randomised->set_volume(10);

	engine->music->add_music("waterfall", "../matt_bentley_-_waterfall_body.ogg", "../matt_bentley_-_waterfall_intro.ogg");
	engine->music->add_music("dissipate", "../matt_bentley_-_dissipate.ogg");
	engine->music->play("dissipate", 64);

	// Create sprites:
	plf::sprite *bird_sprite = engine->sprites->new_sprite("bird", plf::LOOP, plf::ALIGN_LEFT, plf::ALIGN_TOP);
	bird_sprite->add_frames_from_tile("../bird_tile.png", 10, 156, 90);

	plf::sprite *explosion_sprite = engine->sprites->new_sprite("explosion", plf::NO_LOOP, plf::ALIGN_LEFT, plf::ALIGN_TOP);
	explosion_sprite->add_frames("../explosion", 15, 45);

	plf::sprite *backing_sprite = engine->sprites->new_sprite("backing", plf::NO_LOOP, plf::ALIGN_LEFT, plf::ALIGN_TOP);
	backing_sprite->add_frame("../background.jpg", 0);

	// Create entity and set parameters:
	plf::entity *bird_entity = engine->entities->new_entity("eagle");

	bird_entity->set_horizontal_flip(true);
	bird_entity->add_state("flying", bird_sprite);
	bird_entity->add_sound_to_state("flying", "random_caws", plf::REPEATED, 0, 2000, 3500);
	bird_entity->add_collision_block_to_state("flying", 40, 40, 60, 60);
	bird_entity->add_movement_to_state<bird_movement>("flying"); // This is a templated function. bird_movement (defined at the top of this .cpp) specifies the type of movement class to add to the state.
	bird_entity->add_state("exploding", explosion_sprite, true);
	bird_entity->set_current_state("flying");

	// Create layers with different scroll timings:
	plf::layer *backing_layer = engine->layers->new_layer("backing", 0, 0.25, 0, 0, 8000, 3000);
	plf::layer *bird_layer1 = engine->layers->new_layer("birds1", 1, 0.5, 0, 0, 8000, 3000);
	plf::layer *bird_layer2 = engine->layers->new_layer("birds2", 2, 1, 0, 0, 8000, 3000);
	plf::layer *bird_layer3 = engine->layers->new_layer("birds3", 3, 1.25, 0, 0, 8000, 3000);

	// Put some objects on the back layer:
	backing_layer->add_background(backing_sprite, 0, 0, 1);
	backing_layer->spawn_entity("eagle1", bird_entity, 400, 250, 0, 0, .4f, 0);
	backing_layer->spawn_entity("eagle2", bird_entity, 200, 200, 500, 250, .25f, 0);

	unsigned int delta = 0;
	double display_x = 0;
	unsigned int num_loops = 0, doloop = 0;

	SDL_PollEvent(&event);

	// Create vector for holding bird collision pairs:
	std::vector< std::pair<plf::entity *, plf::entity *> > collisions;

	// Set up time:
	Uint32 sdl_time = SDL_GetTicks(), time_for_more_spawn;
	

	do
	{
		time_for_more_spawn = sdl_time + 5000;
		
		// Spawn tons of birds:
		for (unsigned int counter = 0; counter != 920; counter++)
		{
			bird_layer1->spawn_entity("eagle_flock", bird_entity, static_cast<unsigned int>(display_x) + plf::xor_rand() % 600, (plf::xor_rand() % 400), plf::xor_rand() % 900, plf::xor_rand() % 1500, (static_cast<double>(plf::xor_rand() % 25) / 100) + .25, 0);
		}

		for (unsigned int counter = 0; counter != 920; counter++)
		{
			bird_layer2->spawn_entity("eagle_flock", bird_entity, static_cast<unsigned int>(display_x) + plf::xor_rand() % 600, (plf::xor_rand() % 400), plf::xor_rand() % 900, plf::xor_rand() % 1500, (static_cast<double>(plf::xor_rand() % 25) / 100) + .55, 1);
		}

		for (unsigned int counter = 0; counter != 920; counter++)
		{
			bird_layer3->spawn_entity("eagle_flock", bird_entity, static_cast<unsigned int>(display_x) + plf::xor_rand() % 600, (plf::xor_rand() % 400), plf::xor_rand() % 900, plf::xor_rand() % 1500, (static_cast<double>(plf::xor_rand() % 25) / 100) + .75, 2);
		}


		do
		{
			// Process changes for entities on all layers:
			engine->layers->update_layers(delta);

			// Check for and process collisions:
			engine->layers->get_all_collisions(collisions);

			for (std::vector< std::pair<plf::entity *, plf::entity *> >::iterator pair_iterator = collisions.begin(); pair_iterator != collisions.end(); ++pair_iterator)
			{
				// Change state for second bird in collision:
				pair_iterator->second->set_current_state("exploding");
				pair_iterator->second->set_sprite_time_offset(plf::xor_rand() % 500);
			}

			collisions.clear();

			// Render everything to renderer surface:
			engine->layers->draw_layers(delta, (int)display_x, 0);
			
			// Add quadtree display:
			bird_layer1->show_quadtree(engine->renderer, (int)display_x, 0, 140, 0, 0);
			bird_layer2->show_quadtree(engine->renderer, (int)display_x, 0, 0, 140, 0);
			bird_layer3->show_quadtree(engine->renderer, (int)display_x, 0, 0, 0, 140);
			
			// Flip the renderer surface to the window:
			engine->renderer->display_frame();
			
			// Change the location of the sound center within the actual game x/y plane, for stereo positioning of entity-generated sounds, based on the movement of the game display:
			engine->sound->set_sound_center((unsigned int)(display_x) + 300, 200);


			display_x += (double)delta / 10.0;
			++num_loops;
			SDL_PollEvent(&event);

			if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
			{
				delete engine;
				return 0;
			}

			// Here is one method for making sure we're not trying to display more than 60 frames per second:
			// Simply burn off any remaining time by repeatedly checking against the clock until at least 14ms has passed.
			// (16ms is approx 1/60 second), allowing 2ms leeway
			// Not a great method if the next frame takes way longer than expected to render.
			
			do
			{
				delta = static_cast<unsigned int>(SDL_GetTicks() - sdl_time);
			} while (delta < 14); 
			
			sdl_time = SDL_GetTicks(); // reset sdl_time for next frame.

		} while (sdl_time < time_for_more_spawn);

	} while (++doloop != 4);

	
	
	// 2 - same situation, different approach to timing, music fade-between:
	
	// Fade into BoC music:
	engine->music->fadebetween("waterfall", 5000, 64);


	do
	{
		SDL_PollEvent(&event);

		if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
		{
			delete engine;
			return 0;
		}

		engine->layers->update_layers(delta);
		engine->layers->draw_layers(delta, (int)display_x, 0);
		engine->renderer->display_frame();

		engine->sound->set_sound_center((const unsigned int)display_x + 300, 200);
		
		// A (often bad) method of making sure we don't display more than 60 fps:
		// Why? because the OS is only guaranteed to use AT LEAST this much time before giving the program control again.
		// So we try and mitigate it by only delaying if the delta is under a certain amount, but that may not work.
		// This method is generally fine if you're not worried about achieving 60fps frame rates, 
		// if you're going for 30fps or lower it's perfectly fine and saves tons of CPU.
		
		delta = static_cast<unsigned int>(SDL_GetTicks() - sdl_time);
		
		if (delta < 10)
		{
			SDL_Delay(14 - delta);
			delta = static_cast<unsigned int>(SDL_GetTicks() - sdl_time);
		}

		sdl_time = SDL_GetTicks();
		display_x += (double)delta / 10.0;
		++num_loops;

	} while (display_x < 3000);

	logmain << "number of loops: " << num_loops << std::endl;



	// 3 - examples of bypassing the intended engine mechanics:
	// artificial multitexture testing
	// drawing both textures and sprites to screen without use of entities or layers
	// use arrow keys on keyboard to move center of rotation for third texture draw
	// demonstration of colormod'ing

	plf::sprite *tree = engine->sprites->new_sprite("tree", plf::LOOP, plf::ALIGN_CENTER, plf::ALIGN_BOTTOM);
	tree->add_frame("../tree1.png", 90); // adding individual frames instead of using number-matching or adding from a tile
	tree->add_frame("../tree2.png", 90);
	tree->add_frame("../tree3.png", 90);
	tree->add_frame("../tree4.png", 90);
	tree->add_frame("../tree5.png", 90);
	tree->add_frame("../tree6.png", 90);
	tree->add_frame("../tree7.png", 90);
	tree->add_frame("../tree8.png", 90);
	plf::rgb colormod = {255, 255, 255}; // For randomly colour-modding this sprite

	SDL_Surface *seed1 = IMG_Load("../tree1.png");

	// Create a multitexture with the texture block sizes artificially set to 20x20. No point to this, 
	// just an example of how the engine can be bypassed to go direct to the individual components in most cases.
	// In the usual case, a multitexture will be automatically used when a texture is larger than the texture atlas size.
	// If you look at the texture atlas textures during runtime in the section after this, you'll notice the background 
	// has been split into several different atlases as it is too large to fit into any single on of them.
	plf::multitexture *seed_texture = new plf::multitexture(engine->renderer, engine->atlas_manager, seed1, 20, 20);

	SDL_FreeSurface(seed1); // This sort of thing is normally handled under the hood by the engine. At this point the surface is already copied in to the atlas so it's no longer needed. Because we're bypassing the engine here we have to do it manually.
	
	double angle;
	SDL_Point location;
	location.x = 400;
	location.y = 200;

	unsigned int counter3 = 0, frame_counter = 1;

	while (counter3++ != 3)
	{
		angle = 0;
		
		while (angle++ != 360)
		{
			SDL_PollEvent(&event);
			
			if(event.type == SDL_KEYDOWN)
			{
				// Move centerpoint of rotation for one of the trees:
				switch(event.key.keysym.sym)
				{
					case SDLK_UP:
						--location.y;
						break;
					case SDLK_DOWN:
						++location.y;
						break;
					case SDLK_LEFT:
						--location.x;
						break;
					case SDLK_RIGHT:
						++location.x;
						break;
					case SDLK_ESCAPE:
						delete engine;
						return 0;
					default:
						break;
				}
			}
			else if (event.type == SDL_QUIT)
			{
				delete engine;
				return 0;
			}



			engine->renderer->clear_renderer();

			seed_texture->draw(0, 50, 1.5, angle, NULL, SDL_FLIP_NONE);
			seed_texture->draw(150, 50, 1.5, angle, NULL, SDL_FLIP_HORIZONTAL);
			seed_texture->draw(300, 50, 1.5, angle, &location, SDL_FLIP_VERTICAL);

			// Modulate colors:
			if (colormod.r == 255)
			{
				if (colormod.g != 0)
				{
					--colormod.g;
				}
				else
				{
					--colormod.r;
				}
				
				if (colormod.b != 255)
				{
					++colormod.b;
				}
			}
			else if (colormod.b == 255)
			{
				if (colormod.r != 0)
				{
					--colormod.r;
				}
				else
				{
					--colormod.b;
				}

				if (colormod.g != 255)
				{
					++colormod.g;
				}
			}
			else
			{
				if (colormod.b != 0)
				{
					--colormod.b;
				}
				else
				{
					--colormod.g;
				}

				if (colormod.r != 255)
				{
					++colormod.r;
				}
			}
			
			
			tree->draw_frame(frame_counter, 400, 400, 2, false, false, 0, 255, &colormod); // Draw sprite directly to screen without use of layers or entities - another example of bypassing the usual way of doing things with the engine
			
			if (++frame_counter == 8)
			{
				frame_counter = 0;
			}

			engine->renderer->display_frame();
			SDL_Delay(20);
		}
		
	}



// 4 - show texture atlas - use up and down arrow keys to flick between different texture atlases:
	SDL_Rect source = {0, 0, 0, 0};

	unsigned int atlas_number = 1;
	const unsigned int num_atlases = engine->atlas_manager->get_number_of_atlases();
	engine->atlas_manager->get_maximum_texture_size(source.w, source.h);
	
	engine->renderer->clear_renderer();
	SDL_RenderCopy(engine->renderer->get(), engine->atlas_manager->get_atlas_texture(atlas_number), &source, &source); // Using source as destination coordinates here
	engine->renderer->display_frame();
	
	
	while(true)
	{
		SDL_PollEvent(&event);

		if(event.type == SDL_KEYDOWN)
		{
			//Select surfaces based on key press
			switch(event.key.keysym.sym)
			{
				case SDLK_UP:
					atlas_number = (atlas_number == num_atlases) ? atlas_number: atlas_number + 1;
					break;
				case SDLK_DOWN:
					atlas_number = (atlas_number == 1) ? atlas_number: atlas_number - 1;
					break;
				case SDLK_ESCAPE:
					delete engine;
					return 0;
				default:
					break;
			}

			engine->renderer->clear_renderer();
			SDL_RenderCopy(engine->renderer->get(), engine->atlas_manager->get_atlas_texture(atlas_number), &source, &source); // Using source as destination coordinates here
			engine->renderer->display_frame();
		}
		else if(event.type == SDL_QUIT)
		{
			delete engine;
			return 0;
		}

		SDL_Delay(150);
	}

	
	
	// Cleanup - this point will actually never be reached because of the exit conditions in the above loop:
	
	// You don't have to worry about deleting sprites, textures, etc - the engine and subcomponent destructors will do it all for you.
	delete engine; 

	return 0;
}
