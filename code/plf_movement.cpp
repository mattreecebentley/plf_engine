#include <SDL2/SDL.h>

#include "plf_movement.h"
#include "plf_math.h"


namespace plf
{


movement::movement()
{
	current_acceleration.clear();
	current_velocity.clear();
	friction.clear();
	constant_acceleration.clear();
}



void movement::load_values(const double_xy &acceleration, const double_xy &velocity, const double_xy &environment_friction, const double_xy &environment_constant_acceleration)
{
	current_acceleration = acceleration;
	current_velocity = velocity;
	friction = environment_friction;
	constant_acceleration = environment_constant_acceleration;
}



void movement::default_update(double &current_x, double &current_y, const unsigned int delta_time, const unsigned int movement_time, const double resize_movement)
{
	// Apply friction:

	if (friction.x != 0 && current_acceleration.x != 0)
	{
		if (current_acceleration.x < 0)
		{
			current_acceleration.x += friction.x * delta_time;
			
			if (current_acceleration.x > 0)
			{
				current_acceleration.x = 0;
			}
		}
		else // is greater than 0
		{
			current_acceleration.x -= friction.x * delta_time;
			
			if (current_acceleration.x < 0)
			{
				current_acceleration.x = 0;
			}
		}
	}

	if (friction.y != 0 && current_acceleration.y != 0)
	{
		if (current_acceleration.y < 0)
		{
			current_acceleration.y += friction.y * delta_time;
			
			if (current_acceleration.y > 0)
			{
				current_acceleration.y = 0;
			}
		}
		else // is greater than 0 - implied by both if expressions 
		{
			current_acceleration.y -= friction.y * delta_time;

			if (current_acceleration.y < 0)
			{
				current_acceleration.y = 0;
			}
		}
	}


	int milliseconds_left;
	int milliseconds_to_apply;

	// Apply current acceleration impulses:
	if (!(acceleration_impulses.empty()))
	{
		for (std::vector<impulse>::iterator impulse_iterator = acceleration_impulses.begin(); impulse_iterator != acceleration_impulses.end();)  // Iteration occurs in loop, since iterator position may be removed (which counts as an iteration)
		{
			milliseconds_left = static_cast<int>((*impulse_iterator).milliseconds) - static_cast<int>(delta_time);
			milliseconds_to_apply = static_cast<int>(delta_time);

			if (milliseconds_left <= 0) // Impulse is finished after update
			{
				milliseconds_to_apply -= milliseconds_left;
			}

			current_acceleration.x += (*impulse_iterator).x * milliseconds_to_apply;
			current_acceleration.y += (*impulse_iterator).y * milliseconds_to_apply;
			
			if (milliseconds_left > 0)
			{
				++impulse_iterator;
			}
			else // if (milliseconds_left <= 0) - Impulse is finished after update
			{
				impulse_iterator = acceleration_impulses.erase(impulse_iterator); // Remove impulse from vector
			}
		}
	}


	// Apply accelerations:
	current_velocity.x += (current_acceleration.x + constant_acceleration.x) * static_cast<double>(delta_time);
	current_velocity.y += (current_acceleration.y + constant_acceleration.y) * static_cast<double>(delta_time);

	current_x += current_velocity.x;
	current_y += current_velocity.y;

	
	// Apply current velocity impulses:
	if (!(velocity_impulses.empty()))
	{
		for (std::vector<impulse>::iterator impulse_iterator = velocity_impulses.begin(); impulse_iterator != velocity_impulses.end();)  // Iteration occurs in loop, since iterator position may be removed (which counts as an iteration)
		{
			milliseconds_left = (*impulse_iterator).milliseconds - delta_time;
			milliseconds_to_apply = delta_time;
			
			if (milliseconds_left < 0) // Impulse is finished after update
			{
				milliseconds_to_apply += milliseconds_left;
			}
			
			current_x += (*impulse_iterator).x * static_cast<double>(milliseconds_to_apply);
			current_y += (*impulse_iterator).y * static_cast<double>(milliseconds_to_apply);

			if (milliseconds_left > 0)
			{
				++impulse_iterator;
			}
			else // if (milliseconds_left <= 0) - Impulse is finished after update
			{
				impulse_iterator = velocity_impulses.erase(impulse_iterator); // Remove impulse from vector
			}
		}
	}
	
}



// To be (potentially) overwritten by derived functions:
void movement::update(double &current_x, double &current_y, const unsigned int delta_time, const unsigned int movement_time, const double resize_movement, const bool flip_horizontal, const bool flip_vertical)
{
	if (delta_time == 0)
	{
		return;
	}

	default_update(current_x, current_y, delta_time, movement_time, resize_movement);
}


void movement::add_velocity(double movement_per_millisecond_x, double movement_per_millisecond_y)
{
	current_velocity.x += movement_per_millisecond_x;
	current_velocity.y += movement_per_millisecond_y;
}


void movement::add_acceleration(const double movement_change_per_millisecond_x, const double movement_change_per_millisecond_y)
{
	current_acceleration.x += movement_change_per_millisecond_x;
	current_acceleration.y += movement_change_per_millisecond_y;
}


void movement::add_impulse_velocity(const double movement_per_millisecond_x, const double movement_per_millisecond_y, const unsigned int milliseconds)
{
	impulse new_impulse;
	new_impulse.x = movement_per_millisecond_x;
	new_impulse.y = movement_per_millisecond_y;
	new_impulse.milliseconds = milliseconds;
	
	velocity_impulses.push_back(new_impulse);
}


void movement::add_impulse_acceleration(const double movement_change_per_millisecond_x, const double movement_change_per_millisecond_y, const unsigned int milliseconds)
{
	impulse new_impulse;
	new_impulse.x = movement_change_per_millisecond_x;
	new_impulse.y = movement_change_per_millisecond_y;
	new_impulse.milliseconds = milliseconds;
	
	acceleration_impulses.push_back(new_impulse);
}


double_xy movement::get_current_velocity()
{
	return current_velocity;
}


double_xy movement::get_current_acceleration()
{
	return current_acceleration;
}


}