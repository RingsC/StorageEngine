#include "utils/performance_timer.h"

void writeResToFile(const string &op,double timeOut, uint32 len, uint32 thread_nums, uint32 count)
{

#ifdef WIN32
	ofstream out("D:\\XMLDB\\res.txt", std::ios::out | std::ios::app);
#else
	ofstream out;
	try{
		out.exceptions(ofstream::failbit | ofstream::badbit);
		out.open("/home/fd/res.txt", std::ios::out | std::ios::app);
		out.exceptions(std::ofstream::goodbit);
	} catch(ofstream::failure const &ex)
	{
		cout << ex.what() << endl;
		cout << out.rdstate();
	}
#endif
	if(!out.is_open())
	{
		cout << "open file failed" << endl;
		return;
	}
	out <<op<<":thread_nums:" << thread_nums<< "  ;lines:" << count << "  ,cost:" << timeOut  <<"  ,tupleLen:" <<len<< std::endl;
	out.close();
}

