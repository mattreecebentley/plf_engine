#include <sstream>
#include <ctime>

namespace plf
{

	std::string get_timedate_string()
	{
		time_t rawtime;
		struct tm * timeinfo;
		char buffer[80];
	
		time(&rawtime);
		timeinfo = localtime(&rawtime);
	
		strftime(buffer, 80, "%c", timeinfo);
	
		std::ostringstream timedate;
		timedate << buffer;
		return timedate.str();
	}

}