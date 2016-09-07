#ifndef PLF_SOUND_H
#define PLF_SOUND_H

#include <deque>
#include <vector>
#include <map>
#include <string>

#include "plf_stack.h"
#include "plf_colony.h"
#include "plf_math.h"

#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL.h>



namespace plf
{

class sound
{
private:
	Mix_Chunk *sample;
public:
	sound(const char *file_name);
	sound(): sample(NULL) {}; // For inherited entities because c++ inheritance is... just... just, terrible... anyway, this means when the derived class is destructed and calls the base class destructor (after it calls it's own) everything won't explode in a shower of horrific goo
	virtual ~sound();
	virtual int play(const int channel, const bool loop);
	virtual int fadein_play(const int channel, const bool loop, const unsigned int milliseconds);
	virtual int set_volume(const Uint8 volume);
	virtual Uint8 get_volume();
	virtual unsigned int get_length();
};




class sound_manager; // Forward declaration for classes below


// An 'alternating_sound' allows the developer to specify a set of sounds, which when played, will play each sound after the other in sequential order:
class alternating_sound : public sound
{
private:
	std::vector<sound *> sounds;
	std::vector<sound *>::iterator current_sound_iterator;
	plf::sound_manager *sound_manager;
	Uint8 current_volume;
public:
	alternating_sound(plf::sound_manager *_sound_manager);
	~alternating_sound();
	int add_sound(const std::string &sound_id, const int insertion_position = -1); // Default insertion is at end of vector, -1 indicates this. Can be overridden to specify an earlier insertion point
	int play(const int channel, const bool loop);
	int fadein_play(const int channel, const bool loop, const unsigned int milliseconds);
	int set_volume(const Uint8 volume);
	Uint8 get_volume();
	unsigned int get_length(); // Only returns length of currently-selected sound, not all sounds
};



// A 'random_sound' allows the developer to specify a set of sounds, which when played, will randomly choose within the set as to which sound to play. You can specify the probability of a given sound being used over others.
class random_sound : public sound
{
private:
	struct randomised_sound
	{
		plf::sound *sound;
		Uint8 random_chance;
	};

	std::vector<randomised_sound> sounds;
	plf::sound *previous_sound;
	plf::sound_manager *sound_manager;
	unsigned int random_chance_sum; // The total sum of all random_chance's in all randomised_sounds in the 'sounds' vector. Used to calculate actual probabilities,
	bool repeats_allowed; // Allow the same sound to play twice (or more)
	Uint8 current_volume;

public:
	random_sound(plf::sound_manager *_sound_manager, const bool _repeats_allowed = true);
	virtual ~random_sound();
	int add_sound(const std::string &sound_id, const Uint8 random_chance);
	int play(const int channel, const bool loop);
	int fadein_play(const int channel, const bool loop, const unsigned int milliseconds);
	int set_volume(const Uint8 volume);
	Uint8 get_volume();
	unsigned int get_length(); // Only returns length of currently-selected sound
};



enum SOUND_REFERENCE_TYPE
{
	ONE_SHOT, // plays once
	REPEATED, // plays over and over again, but with a delay
	LOOPED // looped
};



// A sound reference is primarily used by the entity class, but can be used separately. It provides a reference to a particular sound, with metadata about that particular *reference* - so that different entities can be playing the same sound and have different timing, volume of that sound, etc. You may think of it as the plf::sound being the stone, and the plf::sound_reference being the slingshot that throws it.
class sound_reference
{
private:
	plf::sound_manager *sound_manager;
	plf::sound *sound_ref;

	SDL_Point last_sound_center, last_distance_from_center;
	unsigned int last_distance; // Last vector distance
	int last_x_distance;	// Last horizontal distance

	int current_channel; // -2 means no channel assigned
	SOUND_REFERENCE_TYPE type;
	int delay_remaining;
	unsigned int initial_delay; // Delay before playing in ms
	unsigned int between_delay; // For repeated sounds, delay between playbacks
	unsigned int delay_random; // For repeated sounds, random delay between playbacks - or before playbacks - adds a randomised ms of up to this number to the between_delay
	bool playing, // class instance has begun playback
		 paused, // class instance is paused
		 loop, // class instance is looping (optimisation to bool of the SOUND_REFERENCE_TYPE, compares once instead of each time sound is played)
		 delaying, // class instance is currently delaying before playing the sound
		 fading_out, // class instance is currently playing, but fading out
		 started;  // class instance has or has not been played yet (at least once)
	Uint8 current_volume, // max volume = 127
		current_pan; // centre = 127

	void set_volume_and_pan();
	void recalculate_volume_and_pan(int x, int y);
public:
	sound_reference();
	void initialise(plf::sound_manager *_sound_manager, plf::sound *_sound, const SOUND_REFERENCE_TYPE sound_type, const unsigned int delay_before_playing, const unsigned int tween_delay, const unsigned int delay_random_element);
	sound_reference(plf::sound_manager *_sound_manager, plf::sound *_sound, const SOUND_REFERENCE_TYPE _type, const unsigned int delay_before_playing = 0, const unsigned int tween_delay = 0, const unsigned int delay_random_element = 0);
	~sound_reference();
	void play(const int x, const int y);
	void fadein_play(const int x, const int y, const unsigned int milliseconds);
	int update(const unsigned int delta_time, const int x, const int y); // Returns 20 when sound finished
	void fadeout(const unsigned int milliseconds);
	void pause();
	void resume();
	void stop();
};




class sound_manager
{
private:
	plf::stack<unsigned int> free_channels;
	plf::colony<unsigned int> one_shot_channels;
	std::map<std::string, sound *> sounds;

	SDL_Point sound_center;
	unsigned int currently_allocated_number_of_channels;
	unsigned int audibility_radius, stereo_radius;

	unsigned int get_free_channel();
	void return_channel(const unsigned int channel);

	friend class sound;
	friend class sound_reference;
	friend class music;

public:
	sound_manager(const unsigned int screen_width, const unsigned int screen_height, const unsigned int initial_number_of_channels = 32);
	~sound_manager();
	void add_sound(const std::string &id, const char *file_name);
	void play_sound(const std::string &id, const Uint8 volume, const Uint8 pan);
	void play_sound_location(const std::string &id, const int x, const int y);
	alternating_sound * add_alternating_sound(const std::string &id);
	random_sound * add_random_sound(const std::string &id, const bool repeats_allowed = true);
	sound * get_sound(const std::string &id);
	void resume_all_sounds();
	void pause_all_sounds();
	void stop_all_sounds();
	void set_sound_center(const int x, const int y);
	void get_current_sound_center(int &x, int &y);
	void set_audibility_radius(const unsigned int radius);
	unsigned int get_audibility_radius();
	void set_stereo_radius(const unsigned int radius);
	unsigned int get_stereo_radius();
	void remove_sound(const std::string &id);
};



}

#endif // sound_H

