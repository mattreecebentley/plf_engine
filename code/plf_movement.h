#ifndef PLF_MOVEMENT_H
#define PLF_MOVEMENT_H

#include <vector>

#include "plf_math.h"


namespace plf
{


	class movement
	{
	protected:
		struct impulse // apply this x_y movement per millisecond, for each millisecond specified in the 'milliseconds' field
		{
			double x, y;
			unsigned int milliseconds;
	
			inline void clear()
			{
				x = 0;
				y = 0;
				milliseconds = 0;
			}
		};
	
		std::vector<impulse> velocity_impulses;
		std::vector<impulse> acceleration_impulses;
		double_xy current_acceleration;
		double_xy current_velocity;
		double_xy friction;
		double_xy constant_acceleration; // ie. Gravity in most cases
	
	public:
		movement();
		virtual ~movement() {};
		virtual movement * clone() { return new movement(); }; // This must be defined for every derived class, as when this movement is assigned to the state of an entity, it must be able to supply a copy of itself.
	
		// When changing from one entity state to the next, one might choose to carry over the current velocity, acceleration etc of the current state. This is what the function below is for:
	   void load_values(const double_xy &acceleration, const double_xy &velocity, const double_xy &environment_friction, const double_xy &environment_constant_acceleration);
	
	   // This processes current velocity, acceleration etc, as well as any impulses that've been added to the object:
		void default_update(double &current_x, double &current_y, const unsigned int delta_time, const unsigned int movement_time, const double resize_movement = 1);
	
		virtual void update(double &current_x, double &current_y, const unsigned int delta_time, const unsigned int movement_time, const double resize_movement = 1, const bool flip_horizontal = false, const bool flip_vertical = false);
	
		void add_velocity(const double movement_per_millisecond_x, const double movement_per_millisecond_y);
		void add_acceleration(const double movement_change_per_millisecond_x, const double movement_change_per_millisecond_y);
		void add_impulse_velocity(const double movement_per_millisecond_x, const double movement_per_millisecond_y, const unsigned int milliseconds);
		void add_impulse_acceleration(const double movement_change_per_millisecond_x, const double movement_change_per_millisecond_y, const unsigned int milliseconds);
	
		inline void clear_acceleration_impulses() { acceleration_impulses.clear(); }; // Removes all acceleration impulses only
		inline void clear_velocity_impulses() { velocity_impulses.clear(); }; // Removes all velocity impulses only
		inline void clear_impulses() { clear_velocity_impulses(); clear_acceleration_impulses(); }; // Removes all velocity and acceleration impulses
		inline void clear_friction() { friction.clear(); };
		inline void clear_constant_acceleration() { constant_acceleration.clear(); };
		inline void clear_current_velocity_and_acceleration() { current_velocity.clear(); current_acceleration.clear(); };
		inline void clear_current_physics() { clear_friction(); clear_constant_acceleration(); clear_current_velocity_and_acceleration(); };
		inline void clear_all_physics() { clear_impulses(); clear_current_physics(); }; // Remove all impulse/velocity/acceleration/etc
	
	   // This can be used by the developer for whatever purpose:
	   void return_all_data(double_xy &acceleration, double_xy &velocity, std::vector<impulse> velocity_impulses, std::vector<impulse> acceleration_impulses);
	
		double_xy get_current_velocity();
		double_xy get_current_acceleration();
	};
	
	
	
	// This basic movement allows the user to add impulses, set velocities, acceleration etc. It should be usable for most objects:
	class default_movement : public movement
	{
	public:
		default_movement() {};
		default_movement * clone() { return new default_movement(); };
	
		void update(double &current_x, double &current_y, const unsigned int delta_time, const unsigned int movement_time, const double resize_movement = 1, const bool horizontal_flip = false, const bool vertical_flip = false)
		{
			default_update(current_x, current_y, delta_time, movement_time, resize_movement);
		}
	};

}

#endif // movement_H
