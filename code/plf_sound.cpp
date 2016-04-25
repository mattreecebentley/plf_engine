//TODO?: change fadein_play on sound_reference to a variable, which the play function then reads

#include <vector>
#include <deque>
#include <map>
#include <cmath> // sound positioning - sqrt
#include <cassert>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "plf_sound.h"
#include "plf_utility.h"


namespace plf
{

sound::sound(const char *file_name)
{
	sample = Mix_LoadWAV(file_name);

	plf_fail_if (sample == NULL, "plf::sound constructor error: sound file " << file_name << " not loaded. SDL_Mix error:" << Mix_GetError());
}



sound::~sound()
{
	Mix_FreeChunk(sample);
}



int sound::play(const int channel, const bool loop)
{
	int looping = 0;

	if (loop)
	{
		looping = -1; // Loop forever
	}

	Mix_PlayChannel(channel, sample, looping);
	return 0;
}



int sound::fadein_play(const int channel, const bool loop, const unsigned int milliseconds)
{
	int looping = 0;

	if (loop)
	{
		looping = -1; // Loop forever
	}

	Mix_FadeInChannel(channel, sample, looping, static_cast<int>(milliseconds));
	return 0;
}



int sound::set_volume(const Uint8 volume)
{
	assert(volume <= 128); // Should be between 0 and 128

	Mix_VolumeChunk(sample, volume);
	return 0;
}



Uint8 sound::get_volume()
{
	return Mix_VolumeChunk(sample, -1);
}



unsigned int sound::get_length()
{
	return static_cast<unsigned int>(static_cast<double>(sample->alen) / 176.4);
}




alternating_sound::alternating_sound(plf::sound_manager *_sound_manager):
	current_sound_iterator(sounds.end()),
 	sound_manager(_sound_manager),
	current_volume(128)
{
	assert(sound_manager != NULL);
}



alternating_sound::~alternating_sound()
{
}



int alternating_sound::add_sound(const std::string &sound_id, const int insertion_position)
{
	sound *sound_to_add = sound_manager->get_sound(sound_id);

	plf_assert(sound_to_add != NULL, "alternating_sound add_sound error: cannot find sound with id '" << sound_id << "'.");
	
	if (insertion_position < 0 || (insertion_position >= static_cast<int>(sounds.size()))) // default, insert at end
	{
		sounds.push_back(sound_to_add);
	}
	else
	{
		sounds.insert(sounds.begin() + insertion_position, sound_to_add);
	}

	current_sound_iterator = sounds.begin(); // iterator may be invalidated with every insertion
	return 0;
}



int alternating_sound::play(const int channel, const bool loop)
{
	assert(!sounds.empty());

	(*current_sound_iterator)->play(channel, loop);
	++current_sound_iterator; // increment to next sound

	if (current_sound_iterator == sounds.end())
	{
		current_sound_iterator = sounds.begin(); // reset to first sound
	}
	
	return 0;
}



int alternating_sound::fadein_play(const int channel, const bool loop, const unsigned int milliseconds)
{
	assert(!(sounds.empty()));
	assert(current_sound_iterator != sounds.end());

	(*current_sound_iterator)->fadein_play(channel, loop, milliseconds);
	++current_sound_iterator; // increment to next sound

	if (current_sound_iterator == sounds.end())
	{
		current_sound_iterator = sounds.begin(); // reset to first sound
	}
	
	return 0;
}



int alternating_sound::set_volume(const Uint8 volume) 
{
	assert(!(sounds.empty()));
	assert(volume <= 128); // Should be between 0 and 128

	current_volume = volume;

	for (std::vector<sound *>::iterator sound_iterator = sounds.begin(); sound_iterator != sounds.end(); ++sound_iterator)
	{
		(*sound_iterator)->set_volume(current_volume);
	}

	return 0;
}




Uint8 alternating_sound::get_volume()
{
	assert(!(sounds.empty()));

	return current_volume;
}




unsigned int alternating_sound::get_length()
{
	assert(!(sounds.empty()));
	assert(current_sound_iterator != sounds.end());
	
	return (*current_sound_iterator)->get_length();
}




random_sound::random_sound(plf::sound_manager *_sound_manager, const bool _repeats_allowed):
	previous_sound(NULL),
	sound_manager(_sound_manager),
	random_chance_sum(0),
	repeats_allowed(_repeats_allowed),
	current_volume(128)
{
	assert(sound_manager != NULL);
}



random_sound::~random_sound()
{
}



int random_sound::add_sound(const std::string &sound_id, const Uint8 random_chance)
{
	sound *sound_to_add = sound_manager->get_sound(sound_id);
	
	plf_assert(sound_to_add != NULL, "random_sound add_sound error: cannot find sound with id '" << sound_id << "'.");
	plf_assert(random_chance != 0, "random_sound add_sound error: for sound with id '" << sound_id << "' supplied random chance is == 0.");
	
	randomised_sound new_randomised_sound;
	new_randomised_sound.random_chance = random_chance;
	new_randomised_sound.sound = sound_to_add;
	
	sounds.push_back(new_randomised_sound);

	random_chance_sum += random_chance;
	
	return 0;
}



int random_sound::play(const int channel, const bool loop)
{
	assert(!(sounds.empty()));

	if (sounds.size() == 1)
	{
		previous_sound = sounds.begin()->sound;
		previous_sound->play(channel, loop);
		return 0;
	}

	unsigned int random_number = rand() % random_chance_sum;
	unsigned int current_chance_level = 0;
	sound *sound_to_play = NULL;
	
	for (std::vector<randomised_sound>::iterator sound_iterator = sounds.begin(); sound_iterator != sounds.end(); ++sound_iterator)
	{
		current_chance_level += (*sound_iterator).random_chance;
		
		if (current_chance_level > random_number) // we have a match
		{
			sound_to_play = (*sound_iterator).sound;
			break;
		}
	}
	
	
	plf_assert(sound_to_play != NULL, "random_sound play error: could not find sound to play. Exiting.");
	
	sound_to_play->play(channel, loop);
	previous_sound = sound_to_play;
	
	return 0;
}



int random_sound::fadein_play(const int channel, const bool loop, const unsigned int milliseconds)
{
	assert(!(sounds.empty()));

	if (sounds.size() == 1)
	{
		previous_sound = sounds.begin()->sound;
		previous_sound->play(channel, loop);
		return 0;
	}


	unsigned int random_number = rand() % random_chance_sum;
	unsigned int current_chance_level = 0;
	sound *sound_to_play = NULL;

	for (std::vector<randomised_sound>::iterator sound_iterator = sounds.begin(); sound_iterator != sounds.end(); ++sound_iterator)
	{
		current_chance_level += (*sound_iterator).random_chance;

		if (current_chance_level > random_number) // we have a match
		{
			sound_to_play = (*sound_iterator).sound;
			break;
		}
	}
	
	
	assert(sound_to_play != NULL); // could not find sound to play ie. something went wrong

	sound_to_play->fadein_play(channel, loop, milliseconds);
	previous_sound = sound_to_play;
	
	return 0;
}



int random_sound::set_volume(const Uint8 volume) // Should be between 0 and 128
{
	assert(!(sounds.empty()));
	assert(volume <= 128); // Should be between 0 and 128

	current_volume = volume;

	for (std::vector<randomised_sound>::iterator sound_iterator = sounds.begin(); sound_iterator != sounds.end(); ++sound_iterator)
	{
		(*sound_iterator).sound->set_volume(current_volume);
	}
	
	return 0;
}




Uint8 random_sound::get_volume()
{
	return current_volume;
}



unsigned int random_sound::get_length()
{
	assert(previous_sound != NULL);
	return previous_sound->get_length();
}




sound_reference::sound_reference():
	last_distance(0),
	last_x_distance(0),
	current_channel(-2),
	delay_remaining(0),
	playing(false),
	paused(false),
	delaying(false),
	fading_out(false),
	started(false),
	current_volume(127), 
	current_pan(127) 
{
	last_sound_center.x = 0;
	last_sound_center.y = 0;
	last_distance_from_center.x = 0;
	last_distance_from_center.y = 0;
}



void sound_reference::initialise(plf::sound_manager *_sound_manager, plf::sound *_sound, const SOUND_REFERENCE_TYPE sound_type, const unsigned int delay_before_playing, const unsigned int tween_delay, const unsigned int delay_random_element)
{
	assert(_sound_manager != NULL);
	assert(_sound != NULL);

	loop = (type == LOOPED);
	sound_manager = _sound_manager;
	sound_ref = _sound;
	type = sound_type;
	initial_delay = delay_before_playing;
	between_delay = tween_delay;
	delay_random = delay_random_element;
}



sound_reference::sound_reference(plf::sound_manager *_sound_manager, plf::sound *_sound, const SOUND_REFERENCE_TYPE sound_type, const unsigned int delay_before_playing, const unsigned int tween_delay, const unsigned int delay_random_element):
	sound_manager(_sound_manager),
	sound_ref(_sound),

	last_distance(0),
	last_x_distance(0),
	current_channel(-2),
	type(sound_type),
	delay_remaining(0),
	initial_delay(delay_before_playing),
	between_delay(tween_delay),
	delay_random(delay_random_element),

	playing(false),
	paused(false),
	delaying(false),
	fading_out(false),
	started(false),

	current_volume(127), 
	current_pan(127) 
{
	assert(sound_manager != NULL);
	assert(sound_ref != NULL);

	loop = (type == LOOPED);
	last_sound_center.x = 0;
	last_sound_center.y = 0;
	last_distance_from_center.x = 0;
	last_distance_from_center.y = 0;
}



sound_reference::~sound_reference()
{
	stop();
}



void sound_reference::play(const int x, const int y)
{
	playing = true;
	started = true;
	paused = false;

	if (current_channel == -2)
	{
		current_channel = sound_manager->get_free_channel();
	}

	recalculate_volume_and_pan(x, y);

	// set volume and stereo pan of channel:
	Mix_Volume(current_channel, current_volume);
	Mix_SetPanning(current_channel, 255 - current_pan, current_pan);

	if (type == REPEATED && !delaying) // start initial delay for repeated sounds
	{
		delaying = true;
		delay_remaining = initial_delay;
		
		if (delay_random != 0) // to avoid modulo by zero
		{
			delay_remaining += rand() % delay_random;
		}

		return;
	}

	sound_ref->play(current_channel, loop);
}



void sound_reference::fadein_play(const int x, const int y, const unsigned int milliseconds)
{
	playing = true;
	started = true;
	paused = false;

	if (current_channel == -2)
	{
		current_channel = sound_manager->get_free_channel();
	}

	recalculate_volume_and_pan(x, y);

	// set volume and stereo pan of channel:
	Mix_Volume(current_channel, current_volume);
	Mix_SetPanning(current_channel, 255 - current_pan, current_pan);

	sound_ref->fadein_play(current_channel, loop, milliseconds);
}



void sound_reference::recalculate_volume_and_pan(const int x, const int y)
{
	SDL_Point current_sound_center;
	sound_manager->get_current_sound_center(current_sound_center.x, current_sound_center.y);
	
	// Figure out volume and stereo pan based on distance between the entity creating this sound and the 'sound center', whether that center be the middle of the screen or where the player is, or some other arbitrary point-
	// Calculate hypotenuse to get radial distance:
	double x_distance = static_cast<double>(x - current_sound_center.x);
	double x2 = x_distance;
	x2 *= x2;
	double y2 = static_cast<double>(y - current_sound_center.y);
	y2 *= y2;
	unsigned int distance = static_cast<unsigned int>(std::sqrt(x2 + y2));
	
	if (distance != last_distance) // Recalculate volume:
	{
		double audibility_radius = sound_manager->get_audibility_radius();
		current_volume = static_cast<Uint8> (((audibility_radius - distance) / audibility_radius) * 128);
		last_distance = distance;

		Mix_Volume(current_channel, current_volume);
	}
	
	if (x_distance != last_x_distance) // Recalculate stereo position:
	{
		if (x_distance == 0) // Fringe case
		{
			last_x_distance = 0;
			current_pan = 128;
		}
		else
		{
			double stereo_radius = sound_manager->get_stereo_radius();
			double signposneg = 1;
			
			if (x_distance < 0)
			{
				x_distance *= -1; // Make positive for arithmetic purposes
				signposneg = -1;
			}
			
			current_pan = static_cast<Uint8> (((x_distance / stereo_radius) * 127.5 * signposneg) + 127.5);
			last_x_distance = static_cast<int>(x_distance * signposneg);
		}

		Mix_SetPanning(current_channel, 255 - current_pan, current_pan);
	}
}



int sound_reference::update(const unsigned int delta_time, const int x, const int y)
{
	if (!playing || current_channel == -2)
	{
		if (!started)
		{
			started = true;
			play (x, y);
			return 0;
		}
		
		return -1;
	}

	if (paused)
	{
		return 0;
	}

	// Sound end conditions (for repeated and oneshot only - loop continues)
	if (Mix_Playing(current_channel) == 0) // has finished playing
	{
		if (type == ONE_SHOT)
		{
			sound_manager->return_channel(current_channel); // Release channel to be used by something else
			current_channel = -2;
			playing = false;
			fading_out = false;
			return 20;
		}
		else if (type == REPEATED)
		{
			if (fading_out) //sound has faded out entirely
			{
				sound_manager->return_channel(current_channel); // Release channel to be used by something else
				current_channel = -2;
				playing = false;
				fading_out = false;
				return 20;
			}
			else if (!delaying) // Start the delay
			{
				delaying = true;
				delay_remaining = between_delay;

				if (delay_random != 0) // to avoid modulo by zero
				{
					delay_remaining += rand() % delay_random;
				}

				return 0;
			}

			// Else, Continue the delay
			delay_remaining -= delta_time;

			if (delay_remaining < 0)
			{
				play(x, y);
				delay_remaining += between_delay;

				if (delay_random != 0) // to avoid modulo by zero
				{
					delay_remaining += rand() % delay_random;
				}

				return 0;
			}
		}
	}

	recalculate_volume_and_pan(x, y);
	return 0;
}



void sound_reference::pause()
{
	if (!paused && current_channel != -2)
	{
		Mix_Pause(current_channel);
	}
}


void sound_reference::resume()
{
	if (paused && current_channel != -2)
	{
		Mix_Pause(current_channel);
	}
}



void sound_reference::stop()
{
	if (current_channel >= 0)
	{
		Mix_HaltChannel(current_channel);
		sound_manager->return_channel(current_channel); // Release channel to be used by something else
		current_channel = -2;
	}

	playing = false;
	paused = false;
	fading_out = false;
	delaying = false;
}



void sound_reference::fadeout(const unsigned int milliseconds)
{
	fading_out = true;

	if (current_channel != -2)
	{
		Mix_FadeOutChannel(current_channel, milliseconds);
	}
}





sound_manager::sound_manager(const unsigned int screen_width, const unsigned int screen_height, const unsigned int initial_number_of_channels):
	audibility_radius(screen_width * 2),
	stereo_radius(screen_width / 2)
{
	sound_center.x = screen_width / 2;
	sound_center.y = screen_height / 2;

	currently_allocated_number_of_channels = Mix_AllocateChannels(initial_number_of_channels);

	// initialise deque:
	for (unsigned int channel = 0; channel != currently_allocated_number_of_channels; ++channel)
	{
		free_channels.push(channel);
	}
}



sound_manager::~sound_manager()
{
	stop_all_sounds();

	for (std::map<std::string, sound *>::iterator sound_iterator = sounds.begin(); sound_iterator != sounds.end(); ++sound_iterator)
	{
		delete sound_iterator->second;
	}
}



unsigned int sound_manager::get_free_channel()
{
	if (!(one_shot_channels.empty()))
	{
		// Check the one-shot sound playback channels to see if any of them have expired:
		for (std::vector<unsigned int>::iterator channel_iterator = one_shot_channels.begin(); channel_iterator != one_shot_channels.end(); )
		{
			if (!(Mix_Playing(*channel_iterator) || Mix_Paused(*channel_iterator)))
			{
				channel_iterator = one_shot_channels.erase(channel_iterator);
				return_channel(*channel_iterator);
			}
			else
			{
				++channel_iterator; // Removed from for loop, as if erase occurs above, iterator is already effectively incremented
			}
		}
	}
	
	if (free_channels.empty()) // add another ten channels:
	{
		for (unsigned int channel = currently_allocated_number_of_channels; channel != currently_allocated_number_of_channels + 10; ++channel)
		{
			free_channels.push(channel);
		}
		
		currently_allocated_number_of_channels += 10;
		Mix_AllocateChannels(currently_allocated_number_of_channels);
	}

	unsigned int channel = free_channels.top();
	free_channels.pop();
	
	return channel;
}



void sound_manager::return_channel(const unsigned int channel)
{
	free_channels.push(channel);
}



void sound_manager::add_sound(const std::string &id, const char *file_name)
{
	assert(id != ""); // Empty id not allowed. 
	plf_assert(sounds.find(id) == sounds.end(), "plf::sound_manager add_sound error: sound with id '" << id << "' already exists. Aborting");  
	
	sound *new_sound = new sound(file_name); // File-loading error-handling in constructor
	
	sounds.insert(std::map<std::string, sound *>::value_type(id, new_sound));
}



void sound_manager::play_sound(const std::string &id, const Uint8 volume, const Uint8 pan)
{
	assert(id != ""); // Empty id not allowed. 

	std::map<std::string, sound *>::iterator sound = sounds.find(id);
	plf_assert(sound != sounds.end(), "plf::sound_manager play_sound error: sound with id '" << id << "' not found. Aborting");
	
	unsigned int sound_channel = get_free_channel(); // This is returned in the next get_free_channel call once the sound has finished playing
	one_shot_channels.push_back(sound_channel); 

	sound->second->play(sound_channel, 0);
}



void sound_manager::play_sound_location(const std::string &id, const int x, const int y)
{
	assert(id != ""); // Empty id not allowed. 

	std::map<std::string, sound *>::iterator sound = sounds.find(id);
	plf_assert(sound != sounds.end(), "plf::sound_manager play_sound_location error: sound with id '" << id << "' not found. Aborting");
	
	// Following section adapted from sound_reference work:
	// set volume and stereo pan of channel:
	SDL_Point current_sound_center;
	get_current_sound_center(current_sound_center.x, current_sound_center.y);
	
	// Figure out volume and stereo pan based on distance between the location of this sound and the 'sound center', whether that center be the middle of the screen or where the player is, or some other arbitrary point-
	// Calculate hypotenuse to get radial distance:
	Uint8 current_volume, current_pan;
	int x_distance = x - current_sound_center.x;
	double x2 = static_cast<double>(x_distance);
	x2 *= x2;
	double y2 = static_cast<double>(y - current_sound_center.y);
	y2 *= y2;
	unsigned int distance = static_cast<unsigned int>(std::sqrt(x2 + y2));
	

	// Calculate volume:
	double current_audibility_radius = static_cast<double>(audibility_radius);
	current_volume = static_cast<Uint8> (((current_audibility_radius - distance) / current_audibility_radius) * 128);
	
	if (current_volume == 0)
	{
		return; // No need to play sound
	}
	
	
	// Calculate pan:
	if (x_distance == 0) // Fringe case
	{
		current_pan = 128;
	}
	else
	{
		current_pan = static_cast<Uint8> (((x_distance / static_cast<double>(stereo_radius)) * 127.5) + 127.5);
	}

	unsigned int sound_channel = get_free_channel(); // This is returned in the next get_free_channel call once the sound has finished playing
	one_shot_channels.push_back(sound_channel); 

	Mix_Volume(sound_channel, current_volume);
	Mix_SetPanning(sound_channel, 255 - current_pan, current_pan);

	sound->second->play(sound_channel, 0);
}



alternating_sound * sound_manager::add_alternating_sound(const std::string &id)
{
	assert(id != ""); // Empty id not allowed. 
	plf_assert(sounds.find(id) == sounds.end(), "plf::sound_manager add_alternating_sound error: sound with id '" << id << "' already exists. Aborting");  

	alternating_sound *new_sound = new alternating_sound(this);
	
	sounds.insert(std::map<std::string, sound *>::value_type(id, new_sound));
	return new_sound;
}



random_sound * sound_manager::add_random_sound(const std::string &id, const bool repeats_allowed)
{
	assert(id != ""); // Empty id not allowed. 
	plf_assert(sounds.find(id) == sounds.end(), "plf::sound_manager add_random_sound error: sound with id '" << id << "' already exists. Aborting");  
	
	random_sound *new_sound = new random_sound(this, repeats_allowed);
	
	sounds.insert(std::map<std::string, sound *>::value_type(id, new_sound));
	return new_sound;
}



sound * sound_manager::get_sound(const std::string &id)
{
	std::map<std::string, sound *>::iterator found_sound = sounds.find(id);
	plf_assert(found_sound != sounds.end(), "plf::sound_manager get sound error: could not find sound with id '" << id << "'.");

	return found_sound->second;
}



void sound_manager::resume_all_sounds()
{
	Mix_Resume(-1);
}



void sound_manager::pause_all_sounds()
{
	Mix_Pause(-1);
}



void sound_manager::stop_all_sounds()
{
	Mix_HaltChannel(-1);
}



void sound_manager::set_sound_center(const int x, const int y)
{
	sound_center.x = x;
	sound_center.y = y;
}



void sound_manager::get_current_sound_center(int &x, int &y)
{
	x = sound_center.x;
	y = sound_center.y;
}



void sound_manager::set_audibility_radius(const unsigned int radius)
{
	audibility_radius = radius;
}



unsigned int sound_manager::get_audibility_radius()
{
	return audibility_radius;
}



void sound_manager::set_stereo_radius(const unsigned int radius)
{
	stereo_radius = radius;
}



unsigned int sound_manager::get_stereo_radius()
{
	return stereo_radius;
}



void sound_manager::remove_sound(const std::string &id)
{
	const int result = static_cast<int>(sounds.erase(id));
	plf_fail_if(result == 0, "plf::sound_manager erase sound error: could not find sound with id '" << id << "'.");
}

}