#include <math.h>

#include "plf_math.h"


// Additional math functions in use by various libraries:

namespace plf
{


	unsigned int round_down_to_power_of_two(unsigned int x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		++x;
		return x >> 1;
	}
	
	
	
	// Fast xorshift+128 random number generator function (original: https://codingforspeed.com/using-faster-psudo-random-generator-xorshift/)
	unsigned int xor_rand()
	{
		static unsigned int x = 123456789;
		static unsigned int y = 362436069;
		static unsigned int z = 521288629;
		static unsigned int w = 88675123;
		
		const unsigned int t = x ^ (x << 11); 
	
		// Rotate the static values (w rotation in return statement):
		x = y;
		y = z;
		z = w;
	   
		return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
	}
	
	
	
	void rotate_point_around_pivot(double &x, double &y, const double pivot_x, const double pivot_y, double angle)
	{
		// Sanitize angle number:
	
	    // modulo operator output varies from compiler to compiler when dealing with negative numbers - this is a workaround.
		while (angle < 0) 
		{
			angle += 360;
		}
	
		if (angle > 360)
		{
			angle = fmod(angle, 360);
		}
	
		// Convert degree to radians:
		angle *= 3.141592653589793238463 / 180;
	
		const double s = sin(angle);
		const double c = cos(angle);
	
		// translate point back to origin:
		x -= pivot_x;
		y -= pivot_y;
	
		// rotate point
		const double xnew = x * c - y * s;
		const double ynew = x * s + y * c;
	
		// translate point back:
		x = xnew + pivot_x;
		y = ynew + pivot_y;
	}

}
