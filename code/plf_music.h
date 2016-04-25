#ifndef PLF_MUSIC_H
#define PLF_MUSIC_H


#include <vector>
#include <map>
#include <string>

#include <SDL2/SDL_mixer.h>

#include "plf_sound.h"


namespace plf
{

class music_manager; // prior declaration for music



class music
{
private:
	enum music_playback_status
	{
		STOPPED, PAUSED, PAUSED_INTRO, PLAYING, PLAYING_INTRO
	};

	enum music_volume_status
	{
		NORMAL, FADING_IN, FADING_OUT
	};

	plf::music_manager * const music_manager; // parent
	plf::sound_manager * const sound_manager; // for intro sections/fade-betweens
	Mix_Chunk *intro; // Intro (non-looping) sections of music. Also used for cross-fades.
	Mix_Music *sdlmix_music;
	music_playback_status playback_status;
	music_volume_status volume_status;
	int current_volume;
	int intro_channel;
	bool loop;

	virtual void end_intro_play_music();
	
	friend void sound_channel_finished_callback(const int channel);
	
public:
	music(plf::music_manager *_music_manager, plf::sound_manager *_sound_manager, const char *file_name, const char *intro_file_name = NULL);
	music() : music_manager(NULL), sound_manager(NULL), intro(NULL), sdlmix_music(NULL) {}; // For inheriting entities because c++ inheritance means the parent constructor gets called no matter what. Hence, const varibles must be initialized.
	virtual ~music();
	virtual void play(Uint8 volume = 128, const bool looping = false);
	virtual void fadein_play(unsigned int milliseconds, Uint8 volume = 128, const bool looping = false);
	virtual void music_finished();
	virtual void toggle_pause();
	virtual void stop();
	virtual void fadeout(unsigned int milliseconds);
	virtual void set_volume(Uint8 volume);
	virtual int get_volume();

	virtual unsigned int get_intro_length();
	virtual bool has_intro();
	virtual bool is_playing();
};



// Internal function, do not document:
// Intros are played as sounds not streams to allow for fade-betweens between pieces of music
// These are global functions - workarounds for SDL_Mixer callbacks for ending intro playback to start the non-intro (looping) music playing
music * store_music_instance_for_sound_channel_finished_callback(music *pointer_to_store, int &channel_to_store);

void sound_channel_finished_callback(const int channel);


// Internal functions, do not document:
// Intros are played as sounds not streams to allow for fade-betweens between pieces of music
// These are global functions - workarounds for SDL_Mixer callbacks for ending intro playback to start the non-intro (looping) music playing
music * store_music_instance_for_music_finished_callback(music *pointer_to_store);

void music_finished_callback();





class alternating_music : public music
{
private:
	std::vector<music *> musics;
	std::vector<music *>::iterator current_music_iterator;
	plf::music_manager * const music_manager;
	plf::sound_manager * const sound_manager;
	Uint8 current_volume;
	bool loop;

public:
	alternating_music(plf::music_manager *_music_manager, plf::sound_manager *_sound_manager);
	~alternating_music();
	void add_music(const std::string &music_id);

	void play(Uint8 volume = 128, const bool looping = false);
	void fadein_play(unsigned int milliseconds, Uint8 volume = 128, const bool looping = false);
	void music_finished();
	void toggle_pause();
	void stop();
	void fadeout(unsigned int milliseconds);
	void set_volume(Uint8 volume);
	int get_volume();

	unsigned int get_intro_length();
	bool has_intro();
	bool is_playing();
};




class random_music : public music
{
private:
	struct randomised_music
	{
		plf::music *music;
		Uint8 random_chance;
	};

	std::vector<randomised_music> musics;
	plf::music_manager * const music_manager;
	plf::sound_manager * const sound_manager;
	music *current_music;
	music *previous_music;
	unsigned int random_chance_sum;
	Uint8 current_volume;
	bool sequential_repeats_allowed;
	bool loop;

public:
	random_music(plf::music_manager *_music_manager, plf::sound_manager *_sound_manager, const bool _sequential_repeats_allowed = false);
	~random_music();
	void add_music(const std::string &music_id, const Uint8 random_chance);

	void play(Uint8 volume = 128, const bool looping = false);
	void fadein_play(unsigned int milliseconds, Uint8 volume = 128, const bool looping = false);
	void music_finished();
	void toggle_pause();
	void stop();
	void fadeout(unsigned int milliseconds);
	void set_volume(Uint8 volume);
	int get_volume();

	unsigned int get_intro_length(); // Get length of song intro (if any) in milliseconds
	bool has_intro();
	bool is_playing();
};





class music_manager
{
private:
	std::map<std::string, music *> tracks;
	plf::sound_manager * const sound_manager;
	music *current_track, *previous_track;

	void fadebetween_finished_null_previous(); // Used for interaction with music end_intro_play_music

	friend class music; // for access to above function
public:
	music_manager(plf::sound_manager *_sound_manager);
	~music_manager();

	music * add_music(const std::string &id, const char *file_name, const char *intro_file_name = NULL);
	alternating_music * add_alternating_music(const std::string &id);
	random_music * add_random_music(const std::string &id, const bool sequential_repeats_allowed = false);
	music * get_music(const std::string &id);
	music * get_current_music();
	int remove_music(const std::string &id);

	void play(const std::string &id, Uint8 volume = 128, const bool looping = false);
	void fadein_play(const std::string &id, unsigned int milliseconds, Uint8 volume = 128, const bool looping = false);
	void toggle_pause();
	void stop();
	void fadeout(unsigned int milliseconds);
	int fadebetween(const std::string &id, unsigned int milliseconds, Uint8 volume = 128, const bool looping = false);
	void set_volume(Uint8 volume);
	int get_volume();

	unsigned int get_length();
};


}
#endif // music_H
