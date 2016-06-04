#ifndef PLF_UTILITY_H
#define PLF_UTILITY_H

#include <cstdlib>
#include <iostream>

#include <SDL2/SDL.h>

namespace plf
{

// Create a 32-bit, alpha-channeled SDL_Surface in the appropriate endian order for the given platform:
inline SDL_Surface * create_surface(const int width, const int height)
{
	/* SDL interprets each pixel as a 32-bit number, so our masks must depend
	   on the endianness (byte order) of the machine */
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		return SDL_CreateRGBSurface(0, width, height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	#else
		return SDL_CreateRGBSurface(0, width, height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	#endif
}


// Return a string containing the current date and time: primarily for logging.
std::string get_timedate_string();


// Custom assert providing more information than is typically given in C's assert function:
#ifndef NDEBUG
	#define plf_assert(condition, message) \
		{ \
			if (!(condition)) \
			{ \
				std::clog << "Assertion `" #condition "` failed in " << __FILE__ << " line " << __LINE__ << ". " << std::endl << message << std::endl; \
				std::clog.flush(); \
				SDL_Quit(); \
				std::exit(EXIT_FAILURE); \
			} \
		}

#else

	#define plf_assert(condition, message) { }

#endif



// Like a reverse-assert, but doesn't get turned off in non-debug compile modes:
#define plf_fail_if(condition, message) \
	{ \
		if (condition) \
		{ \
			std::clog << "Aborting because `" #condition "` in " << __FILE__ << " line " << __LINE__ << ". " << std::endl << message << std::endl; \
			std::clog << "Last SDL Error code was: " << SDL_GetError() << std::endl; \
			std::clog.flush(); \
			SDL_Quit(); \
			std::exit(EXIT_FAILURE); \
		} \
	}

}


#endif // PLF_UTILITY_H
