#ifndef PLF_LOG_H
#define PLF_LOG_H

#include <fstream>


namespace plf
{

class log
{
private:
	std::ofstream logfile;

public:
	log(const char *logfile_name, const bool append = false);
	~log();

	template <typename logtemplate>
   friend log & operator << (log& log, const logtemplate& value)
	{
		if (log.logfile.is_open())
		{
			log.logfile << value;
			log.logfile.flush();
		}
		return log;
	}


   friend log & operator << (log& log, std::ostream& (*pf) (std::ostream&))
	{
		if (log.logfile.is_open())
		{
			log.logfile << pf;
			log.logfile.flush();
		}
		return log;
	}
};

}

#endif // log_H