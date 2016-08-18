#ifndef PLF_MATH_H
#define PLF_MATH_H

// Additional math functions in use by various libraries:


namespace plf
{


struct double_xy
{
	double x, y;

	void clear()
	{
		x = 0;
		y = 0;
	}
};


unsigned int round_down_to_power_of_two(unsigned int x);


unsigned int xor_rand();


inline unsigned int fast_mod(const unsigned int input, const unsigned int ceiling) // courtesy of chandler carruth
{
    // apply the modulo operator only when needed
    // (i.e. when the input is greater than the ceiling)
    return (input >= ceiling) ? input % ceiling : input;
}


inline unsigned int rand_within(const unsigned int range)
{
	return fast_mod(xor_rand(), range);
}


void rotate_point_around_pivot(double &x, double &y, const double pivot_x, const double pivot_y, double angle);


inline bool is_power_of_two(const unsigned int x)
{
	return ((x != 0) && (x & (x - 1)) == 0);
}



// Standard integer divide rounds down in c++ - this rounds up instead:
inline unsigned int divide_and_round_up(const unsigned int number, const unsigned int divisor)
{
   return (number + (divisor - 1)) / divisor;
}



// Standard integer round:
inline unsigned int divide_and_round(const unsigned int number, const unsigned int divisor)
{
	return ((number * 10) + 5) / (divisor * 10);
}



// Standard integer round:
inline int round_double_to_int(const double number)
{
	return static_cast<int>(number + .5);
}



}
#endif // PLF_MATH_H
