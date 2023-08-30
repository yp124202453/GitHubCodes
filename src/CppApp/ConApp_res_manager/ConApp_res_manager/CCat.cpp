#include "CCat.h"

CCat::~CCat()
{
	cout << __func__ << endl;
}

void CCat::Run()
{
	cout << __FILE__ << " line = " << __LINE__ << " " << __func__ << endl;
}
