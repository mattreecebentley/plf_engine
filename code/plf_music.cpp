#include <vector>
#include <map>
#include <string>
#include <cassert>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>


#include "plf_utility.h"
#include "plf_sound.h"
#include "plf_music.h"
#include "plf_math.h"


namespace plf
{


	// Workaround for SDL_Mixer callback (to make working with intro sounds & fadebetweens possible)...
	music * store_music_instance_for_sound_channel_finished_callback(music *pointer_to_store, int &channel_to_store)
	{
		// stores pointer and channel if pointer supplied isn't NULL, returns pointer and channel if pointer supplied is NULL
		static music *music_pointer;
		static int channel;
	
		if (pointer_to_store == NULL)
		{
			channel_to_store = channel;
			return music_pointer;
		}
		
		if (channel_to_store == -5) // remove all references for safety
		{
			channel = 0;
			music_pointer = NULL;
			return NULL;
		}
		
		channel = channel_to_store;
		music_pointer = pointer_to_store;
		return NULL;
	}
	
	
	
	// If active, checks supplied channel against stored channel (in above function) and if a match, triggers end-conditions in associated music instance:
	void sound_channel_finished_callback(const int channel)
	{
		static bool active;
		
		if (channel == -4) // Activate
		{
			active = true;
			return;
		}
		
		if (!active)
		{
			return;
		}
			
		if (channel == -5) // Disallow future checks for speed purposes
		{
			active = false;
			return;
		}
	
		int correct_channel = 0; // unused anyway because first argument is NULL
		music *music_playing = store_music_instance_for_sound_channel_finished_callback(NULL, correct_channel);
		
		if (music_playing == NULL || channel != correct_channel)
		{
			return;
		}
		
		music_playing->end_intro_play_music();
	}
	
	
	
	// Workaround for SDL_Mixer callback (to make working with random and alternating music tracks)...
	music * store_music_instance_for_music_finished_callback(music *pointer_to_store)
	{
		// stores pointer and channel if pointer supplied isn't NULL, returns pointer and channel if pointer supplied is NULL
		static music *music_pointer;
	
		if (music_pointer == NULL)
		{
			music_pointer = pointer_to_store;
		}
		else if (pointer_to_store == NULL)
		{
			music *temp = music_pointer;
			music_pointer = NULL;
			return temp;
		}
	
		return music_pointer;
	}
	
	
	
	void music_finished_callback()
	{
		music *stored_pointer = store_music_instance_for_music_finished_callback(NULL);
		
		if (stored_pointer == NULL)
		{
			return;
		}
		
		stored_pointer->music_finished();
	}
	
	
	
	music::music(plf::music_manager *_music_manager, plf::sound_manager *_sound_manager, const char *file_name, const char *intro_file_name):
		music_manager(_music_manager),
		sound_manager(_sound_manager),
		intro(NULL),
		sdlmix_music(NULL),
		playback_status(STOPPED),
		volume_status(NORMAL),
		current_volume(128),
		intro_channel(-2),
		loop(false)
	{
		assert(sound_manager != NULL);
		assert(music_manager != NULL);
		
		if (intro_file_name != NULL)
		{
			intro = Mix_LoadWAV(intro_file_name);
			
			plf_fail_if (intro == NULL, "plf::music constructor error: intro sound file " << file_name << " not found/loaded.");
		}
	
		sdlmix_music = Mix_LoadMUS(file_name);
	
		plf_fail_if (sdlmix_music == NULL, "plf::music constructor error: sound file " << file_name << " not found/loaded.");
	}
	
	
	
	
	music::~music()
	{
		if (intro != NULL)
		{
			Mix_FreeChunk(intro);
			intro = NULL;
		}
		
		if (sdlmix_music != NULL)
		{
			Mix_FreeMusic(sdlmix_music);
			sdlmix_music = NULL;
		}
	}
	
	
	
	
	void music::play(Uint8 volume, const bool looping)
	{
		if (playback_status != STOPPED)
		{
			return;
		}
	
		assert(volume <= 128);
	
		volume_status = NORMAL;
		current_volume = static_cast<int>(volume);
		loop = looping;
		
		int sdl_looping = 1;
		
		if (looping)
		{
			sdl_looping = -1;
		}
	
		intro_channel = 0;
	
		if (intro != NULL)
		{
			intro_channel = static_cast<int>(sound_manager->get_free_channel());
			Mix_Volume(intro_channel, current_volume);
	
			Mix_PlayChannel(intro_channel, intro, 0);
			playback_status = PLAYING_INTRO;
			store_music_instance_for_sound_channel_finished_callback(this, intro_channel);
			sound_channel_finished_callback(-4); // Activate sound_channel_finished_callback
			Mix_ChannelFinished(sound_channel_finished_callback);
			return;
		}
		
		
		Mix_VolumeMusic(current_volume);
		
		store_music_instance_for_music_finished_callback(this);
		Mix_PlayMusic(sdlmix_music, sdl_looping);
		playback_status = PLAYING;
	}
	
	
	
	
	void music::fadein_play(unsigned int milliseconds, Uint8 volume, const bool looping)
	{
		if (playback_status != STOPPED)
		{
			return;
		}
	
		volume_status = FADING_IN;
		current_volume = static_cast<int>(volume);
		loop = looping;
		
		const int sdl_looping = (looping) ? -1 : 0;
	
		intro_channel = 0;
	
		if (intro != NULL)
		{
			intro_channel = sound_manager->get_free_channel();
			Mix_Volume(intro_channel, current_volume);
	
			const unsigned int maximum_fade_length = get_intro_length();
			
			if (milliseconds > maximum_fade_length)
			{
				milliseconds = maximum_fade_length;
			}
			
			Mix_FadeInChannel (intro_channel, intro, sdl_looping, static_cast<int>(milliseconds));
			store_music_instance_for_sound_channel_finished_callback(this, intro_channel);
			sound_channel_finished_callback(-4); // Activate sound_channel_finished_callback
			Mix_ChannelFinished(sound_channel_finished_callback);
			playback_status = PLAYING_INTRO;
			return;
		}
		
		
		Mix_VolumeMusic(current_volume);
		
		store_music_instance_for_music_finished_callback(this);
		Mix_FadeInMusic(sdlmix_music, sdl_looping, static_cast<int>(milliseconds));
		playback_status = PLAYING;
		
		return;
	}
	
	
	
	
	void music::end_intro_play_music()
	{
		int pass_value = -5;
		store_music_instance_for_sound_channel_finished_callback(NULL, pass_value);
		sound_channel_finished_callback(-5); // Deactivate sound_channel_finished_callback
		
		music_manager->fadebetween_finished_null_previous();
		Mix_VolumeMusic(current_volume);
	
		int sdl_looping = 1;
	
		if (loop)
		{
			sdl_looping = -1;
		}
	
		store_music_instance_for_music_finished_callback(this);
		Mix_PlayMusic(sdlmix_music, sdl_looping);
	
		sound_manager->return_channel(intro_channel);
		intro_channel = -2;
		playback_status = PLAYING;
		volume_status = NORMAL;
	}
	
	
	
	
	void music::music_finished()
	{
		playback_status = STOPPED;
	}
	
	
	
	
	void music::toggle_pause()
	{
		if (playback_status == PLAYING)
		{
			Mix_PauseMusic();
			playback_status = PAUSED;
		}
		else if (playback_status == PLAYING_INTRO)
		{
			Mix_PauseMusic(); // Just in case another piece of music is fading out
			Mix_Pause(intro_channel);
			playback_status = PAUSED_INTRO;
		}
		else if (playback_status == PAUSED)
		{
			Mix_ResumeMusic();
			playback_status = PLAYING;
		}
		else if (playback_status == PAUSED_INTRO)
		{
			Mix_ResumeMusic(); // Just in case another piece of music is fading out
			Mix_Resume(intro_channel);
			playback_status = PLAYING_INTRO;
		}
	}
	
	
	
	
	void music::stop()
	{
		store_music_instance_for_music_finished_callback(NULL);
		Mix_HaltMusic();
	
		if (playback_status == PLAYING_INTRO || playback_status == PAUSED_INTRO)
		{
			int pass_value = -5;
			store_music_instance_for_sound_channel_finished_callback(NULL, pass_value);
			sound_channel_finished_callback(-5); // Deactivate sound_channel_finished_callback
			Mix_HaltChannel(intro_channel);
			sound_manager->return_channel(intro_channel);
			intro_channel = -2;
		}
		
		playback_status = STOPPED;
	}
	
	
	
	
	void music::fadeout(unsigned int milliseconds)
	{
		if (playback_status == PLAYING)
		{
			Mix_FadeOutMusic(static_cast<int>(milliseconds));
		}
		else if (playback_status == PLAYING_INTRO)
		{
			unsigned int maximum_fade_length = get_intro_length();
			
			if (milliseconds > maximum_fade_length)
			{
				milliseconds = maximum_fade_length;
			}
			
			Mix_FadeOutChannel(intro_channel, milliseconds);
		}
	}
	
	
	
	
	void music::set_volume(Uint8 volume)
	{
		assert(volume <= 128); // Must be between 0 and 128
		
		current_volume = static_cast<int>(volume);
		
		if (intro != NULL)
		{
			Mix_VolumeChunk(intro, static_cast<int>(volume));
		}
		
		Mix_VolumeMusic(current_volume);
	}
	
	
	
	unsigned int music::get_intro_length()
	{
		if (intro == NULL)
		{
			return 0;
		}
		
		return static_cast<int>(static_cast<double>(intro->alen) / 176.4);
	}
	
	
	
	int music::get_volume()
	{
		return current_volume;
	}
	
	
	
	bool music::has_intro()
	{
		return !(intro == NULL);
	}
	
	
	
	bool music::is_playing()
	{
		return (playback_status != STOPPED);
	}
	
	
	
	
	random_music::random_music(plf::music_manager *_music_manager, plf::sound_manager *_sound_manager, const bool _sequential_repeats_allowed):
		music_manager(_music_manager),
		sound_manager(_sound_manager),
		current_music(NULL),
		previous_music(NULL),
		random_chance_sum(0),
		current_volume(128),
		sequential_repeats_allowed(_sequential_repeats_allowed),
		loop(false)
	{
		assert(sound_manager != NULL);
		assert(music_manager != NULL);
	}
	
	
	
	
	random_music::~random_music()
	{
		stop();
	}
	
	
	
	void random_music::add_music(const std::string &music_id, const Uint8 random_chance)
	{
		plf::music *music_to_add = music_manager->get_music(music_id);
	
		plf_assert(music_to_add != NULL, "random_music add_music error: cannot find music with id '" << music_id << "'.");
		plf_assert(random_chance != 0, "random_music add_music error: music with id '" << music_id << "' attempted to be inserted with random_chance == 0.");
		
		randomised_music new_randomised_music;
		new_randomised_music.random_chance = random_chance;
		new_randomised_music.music = music_to_add;
		
		musics.push_back(new_randomised_music);
	
		random_chance_sum += random_chance;
	}
	
	
	
	void random_music::music_finished()
	{
		assert(current_music != NULL); // music finished callback when current_music is NULL - something is wrong.
		
		current_music->music_finished();
		play(current_volume);
	}
	
	
	
	void random_music::play(Uint8 volume, const bool looping)
	{
		assert(!(musics.empty()));
	
		if (musics.size() == 1)
		{
			current_music = musics.begin()->music;
			current_music->play(volume, looping);
			return;
		}
	
		previous_music = NULL;
	
		if (current_music != NULL)
		{
			current_music->stop();
			previous_music = current_music;
			current_music = NULL;
		}
	
		music *music_to_play = NULL;
	
		do
		{
			unsigned int random_number = rand_within(random_chance_sum);
			unsigned int current_chance_level = 0;
			
			for (std::vector<randomised_music>::iterator music_iterator = musics.begin(); music_iterator != musics.end(); ++music_iterator)
			{
				current_chance_level += (*music_iterator).random_chance;
				
				if (current_chance_level > random_number) // we have a match
				{
					music_to_play = (*music_iterator).music;
					break;
				}
			}
		} while (!sequential_repeats_allowed && music_to_play == previous_music); // If sequential repeats are disallowed, keep looping until selected music doesn't match previously-played music
		
		assert(music_to_play != NULL); // something went wrong, this should not happen
		
		store_music_instance_for_music_finished_callback(this);
		current_volume = volume;
		music_to_play->play(volume, looping);
		current_music = music_to_play;
	}
	
	
	
	
	void random_music::fadein_play(unsigned int milliseconds, Uint8 volume, const bool looping)
	{
		assert(!(musics.empty()));
	
		previous_music = NULL;
	
		if (musics.size() == 1)
		{
			current_music = musics.begin()->music;
			current_music->fadein_play(milliseconds, volume, looping);
			return;
		}
	
		if (current_music != NULL)
		{
			current_music->stop();
			previous_music = current_music;
			current_music = NULL;
		}
	
		music *music_to_play = NULL;
		
		do
		{
			unsigned int random_number = rand_within(random_chance_sum);
			unsigned int current_chance_level = 0;
			
			for (std::vector<randomised_music>::iterator music_iterator = musics.begin(); music_iterator != musics.end(); ++music_iterator)
			{
				current_chance_level += (*music_iterator).random_chance;
				
				if (current_chance_level > random_number) // we have a match
				{
					music_to_play = (*music_iterator).music;
					break;
				}
			}
		} while (!sequential_repeats_allowed && music_to_play == previous_music); // If sequential repeats are disallowed, keep looping until selected music doesn't match previously-played music
		
		
		assert(music_to_play != NULL); // something went wrong, this should not happen
		
		store_music_instance_for_music_finished_callback(this);
		current_volume = volume;
		music_to_play->fadein_play(milliseconds, volume, looping);
		current_music = music_to_play;
	}
	
	
	
	
	void random_music::toggle_pause()
	{
		if (current_music == NULL) return;
		current_music->toggle_pause();
	}
	
	
	
	void random_music::stop()
	{
		if (current_music == NULL) return;
		current_music->stop();
	}
	
	
	
	void random_music::fadeout(unsigned int milliseconds)
	{
		if (current_music == NULL) return;
		current_music->fadeout(milliseconds);
	}
	
	
	void random_music::set_volume(Uint8 volume)
	{
		current_volume = volume;
		
		if (current_music == NULL) return;
	
		current_music->set_volume(current_volume);
	}
	
	
	
	int random_music::get_volume()
	{
		return current_volume;
	}
	
	
	unsigned int random_music::get_intro_length()
	{
		if (current_music == NULL) return 0;
	
		return current_music->get_intro_length();
	}
	
	
	
	bool random_music::has_intro()
	{
		if (current_music == NULL) return false;
	
		return current_music->has_intro();
	}
	
	
	
	
	bool random_music::is_playing()
	{
		if (current_music == NULL) return false;
	
		return current_music->is_playing();
	}
	
	
	
	
	alternating_music::alternating_music(plf::music_manager *_music_manager, plf::sound_manager *_sound_manager):
		music_manager(_music_manager),
		sound_manager(_sound_manager),
		current_volume(128),
		loop(false)
	{
		current_music_iterator = musics.end();
		
		assert(sound_manager !=  NULL);
		assert(music_manager !=  NULL);
	}
	
	
	
	
	alternating_music::~alternating_music()
	{
		stop();
	}
	
	
	
	void alternating_music::add_music(const std::string &music_id)
	{
		plf::music *music_to_add = music_manager->get_music(music_id);
		
		plf_assert(music_to_add != NULL, "alternating_music add_music error: cannot find music with id '" << music_id << "'.");
		
		musics.push_back(music_to_add);
		
		current_music_iterator = musics.begin(); // every time because of possible iterator invalidation from push_back
	}
	
	
	
	void alternating_music::music_finished()
	{
		assert((*current_music_iterator) != NULL); // Something's gone wrong, music finished callback when current_music is NULL
		
		(*current_music_iterator)->music_finished();
		play(current_volume);
	}
	
	
	
	void alternating_music::play(Uint8 volume, const bool looping)
	{
		if (current_music_iterator == musics.end()) return;
	
		if (musics.size() == 1)
		{
			(*current_music_iterator)->play(volume, looping);
			return;
		}
	
		if (current_music_iterator != musics.end())
		{
			(*current_music_iterator)->stop();
		}
	
		++current_music_iterator;
		
		if (current_music_iterator == musics.end()) // loop back to first track
		{
			current_music_iterator = musics.begin();
		}
	
		store_music_instance_for_music_finished_callback(this);
		current_volume = volume;
		(*current_music_iterator)->play(volume, looping);
	}
	
	
	
	
	void alternating_music::fadein_play(unsigned int milliseconds, Uint8 volume, const bool looping)
	{
		if (current_music_iterator == musics.end()) return;
	
		if (musics.size() == 1)
		{
			(*current_music_iterator)->play(volume, looping);
			return;
		}
	
		if (current_music_iterator != musics.end())
		{
			(*current_music_iterator)->stop();
		}
		
		++current_music_iterator;
		
		if (current_music_iterator == musics.end()) // loop back to first track
		{
			current_music_iterator = musics.begin();
		}
	
		store_music_instance_for_music_finished_callback(this);
		current_volume = volume;
		(*current_music_iterator)->fadein_play(milliseconds, volume, looping);
	}
	
	
	
	
	void alternating_music::toggle_pause()
	{
		if (current_music_iterator == musics.end()) return;
	
		(*current_music_iterator)->toggle_pause();
	}
	
	
	
	void alternating_music::stop()
	{
		if (current_music_iterator == musics.end()) return;
	
		(*current_music_iterator)->stop();
	}
	
	
	
	void alternating_music::fadeout(unsigned int milliseconds)
	{
		if (current_music_iterator == musics.end()) return;
	
		(*current_music_iterator)->fadeout(milliseconds);
	}
	
	
	void alternating_music::set_volume(Uint8 volume)
	{
		if (current_music_iterator == musics.end()) return;
	
		(*current_music_iterator)->set_volume(volume);
	}
	
	
	
	int alternating_music::get_volume()
	{
		if (current_music_iterator == musics.end()) return 0;
	
		return (*current_music_iterator)->get_volume();
	}
	
	
	unsigned int alternating_music::get_intro_length()
	{
		if (current_music_iterator == musics.end()) return 0;
	
		return (*current_music_iterator)->get_intro_length();
	}
	
	
	
	bool alternating_music::has_intro()
	{
		if (current_music_iterator == musics.end()) return false;
	
		return (*current_music_iterator)->has_intro();
	}
	
	
	
	bool alternating_music::is_playing()
	{
		if (current_music_iterator == musics.end()) return false;
	
		return (*current_music_iterator)->is_playing();
	}
	
	
	
	music_manager::music_manager(plf::sound_manager *_sound_manager):
		sound_manager(_sound_manager),
		current_track(NULL),
		previous_track(NULL)
	{
		assert (sound_manager != NULL);
		
		store_music_instance_for_music_finished_callback(NULL); // Initialise music-finished callback pointer storage system
		Mix_HookMusicFinished(music_finished_callback);
	}
	
	
	
	
	music_manager::~music_manager()
	{
		// Delete all music
		if (!(tracks.empty()))
		{
			for (std::map<std::string, music *>::iterator track_iterator = tracks.begin(); track_iterator != tracks.end(); ++track_iterator)
			{
				delete track_iterator->second;
			}
		}
	}
	
	
	
	
	music * music_manager::add_music(const std::string &id, const char *file_name, const char *intro_file_name)
	{
		assert(id != ""); //id must not be empty
		plf_assert(tracks.find(id) == tracks.end(), "plf::music_manager add_music error: track with id '" << id << "' already exists.");
		
		music *new_music = new music(this, sound_manager, file_name, intro_file_name);
		
		tracks.insert(std::map<std::string, music *>::value_type(id, new_music));
		return new_music;
	}
	
	
	
	
	alternating_music * music_manager::add_alternating_music(const std::string &id)
	{
		assert(id != ""); //id must not be empty
		plf_assert(tracks.find(id) == tracks.end(), "plf::music_manager add_alternating_music error: track with id '" << id << "' already exists.");
		
		alternating_music *new_music = new alternating_music(this, sound_manager);
		
		tracks.insert(std::map<std::string, music *>::value_type(id, new_music));
		return new_music;
	}
	
	
	
	random_music * music_manager::add_random_music(const std::string &id, const bool sequential_repeats_allowed)
	{
		assert(id != ""); //id must not be empty
		plf_assert(tracks.find(id) == tracks.end(), "plf::music_manager add_random_music error: track with id '" << id << "' already exists.");
		
		random_music *new_music = new random_music(this, sound_manager, sequential_repeats_allowed);
		
		tracks.insert(std::map<std::string, music *>::value_type(id, new_music));
		return new_music;
	}
	
	
	
	music * music_manager::get_music(const std::string &id)
	{
		assert(id != ""); //id must not be empty
		
		std::map<std::string, music *>::iterator track_iterator = tracks.find(id);
		
		plf_assert(track_iterator != tracks.end(), "plf::music_manager get_music error: track with id '" << id << "' not found.");
		
		return track_iterator->second;
	}
	
	
	
	music * music_manager::get_current_music()
	{
		return current_track;
	}
	
	
	
	int music_manager::remove_music(const std::string &id)
	{
		assert(id != ""); //id must not be empty
		
		std::map<std::string, music *>::iterator track_iterator = tracks.find(id);
		
		plf_assert(track_iterator != tracks.end(), "plf::music_manager remove_music error: track with id '" << id << "' not found.");
		
		if (track_iterator->second == current_track)
		{
			if (current_track->is_playing())
			{
				std::clog << "plf::music_manager remove_music error: track with id '" << id << "' is currently playing. Aborting" << std::endl;
				return -1;
			}
			
			current_track = NULL;
		}
		
		if (track_iterator->second == previous_track)
		{
			if (previous_track->is_playing())
			{
				std::clog << "plf::music_manager remove_music error: track with id '" << id << "' is currently playing. Aborting" << std::endl;
				return -1;
			}
			
			previous_track = NULL;
		}
	
		tracks.erase(track_iterator);
		
		return 0;
	}
	
	
	
	
	void music_manager::play(const std::string &id, Uint8 volume, const bool looping)
	{
		music *music_to_play = get_music(id);
		music_to_play->play(volume, looping);
		current_track = music_to_play;
	}
	
	
	
	
	void music_manager::fadein_play(const std::string &id, unsigned int milliseconds, Uint8 volume, const bool looping)
	{
		music *music_to_play = get_music(id);
		current_track = music_to_play;
		music_to_play->fadein_play(milliseconds, volume, looping);
	}
	
	
	
	
	int music_manager::fadebetween(const std::string &id, unsigned int milliseconds, Uint8 volume, const bool looping)
	{
		if (current_track == NULL)
		{
			fadein_play(id, milliseconds, volume, looping);
			return 0;
		}
		else if (!(current_track->is_playing()))
		{
			current_track = NULL;
			fadein_play(id, milliseconds, volume, looping);
			return 0;
		}
	
		music *music_to_play = get_music(id);
		
		if (!(music_to_play->has_intro()))
		{
			std::clog << "plf::music_manager fadebetween error: Cannot fade between if new music has no intro portion. Aborting." << std::endl;
			return -1; // Cannot fade between if no intro portion
		}
		
		previous_track = current_track;
		current_track = music_to_play;
		
		int maximum_fade_length = music_to_play->get_intro_length();
		
		if (static_cast<int>(milliseconds) > maximum_fade_length)
		{
			milliseconds = maximum_fade_length;
		}
	
		Mix_FadeOutMusic(milliseconds);
		music_to_play->fadein_play(milliseconds, volume, looping);
		return 0;
	}
	
	
	
	
	void music_manager::fadebetween_finished_null_previous()
	{
		previous_track = NULL;
	}
	
	
	
	void music_manager::toggle_pause()
	{
		if (current_track == NULL) return;
		
		current_track->toggle_pause();  // Just in case there's a fadebetween happening
		
		if (previous_track != NULL)
		{
			previous_track->toggle_pause();
		}
	}
	
	
	
	
	void music_manager::stop()
	{
		if (current_track == NULL) return;
		
		if (previous_track != NULL)  // Just in case there's a fadebetween happening
		{
			previous_track->stop(); 
		}
		
		current_track->stop();
		
		current_track = NULL;
		previous_track = NULL;
	}
	
	
	
	
	void music_manager::fadeout(unsigned int milliseconds)
	{
		if (current_track == NULL) return;
		
		current_track->fadeout(milliseconds);
	}
	
	
	
	
	void music_manager::set_volume(Uint8 volume)
	{
		if (current_track == NULL) return;
		
		current_track->set_volume(volume);
	}
	
	
	
	
	int music_manager::get_volume()
	{
		if (current_track == NULL) return 0;
		
		return current_track->get_volume();
	}

}