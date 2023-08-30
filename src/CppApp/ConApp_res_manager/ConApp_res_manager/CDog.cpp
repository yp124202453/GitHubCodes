#include "CDog.h"

CDog::~CDog()
{
	cout << __func__ << endl;
}

void CDog::Run()
{
	cout << __FILE__ << " line = " << __LINE__ << " " << __func__ << endl;
}
