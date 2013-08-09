#include <embUnit/embUnit.h>

TestRef run_bmp085_tests(void);

int main(int argc, const char* argv[])
{
	TestRunner_start();
	TestRunner_runTest(run_bmp085_tests());
	TestRunner_end();

	return 0;
}
