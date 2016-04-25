#include <fstream>

#include "plf_log.h"
#include "plf_utility.h"



namespace plf
{

log::log(const char *logfile_name, const bool append)
{
	if (append)
	{
		logfile.open(logfile_name, std::ofstream::out | std::ofstream::app);
	}
	else
	{
		logfile.open(logfile_name);
	}

	plf_fail_if (!logfile.is_open(), "plf::log Constructor: Problem! Logfile '" << logfile_name << "' could not be opened. Check filename/path/locking.");
}


log::~log()
{
	logfile.close();
}


}