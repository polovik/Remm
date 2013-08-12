#include <embUnit/embUnit.h>
#include "wiringPiI2C.h"
#include "../server/bmp085.h"

static void setUp(void)
{
}

static void tearDown(void)
{
}

static void test_init(void)
{
	TEST_ASSERT_EQUAL_INT(0, init_bmp085(BMP085_MODE_ULTRALOWPOWER));
}

static void test_get_temperature(void)
{
	float temp;
	int raw_temperature_reg;

    set_testing_module(MODULE_BMP085_TEMPERATURE);
	temp = get_temperature(&raw_temperature_reg);
	TEST_ASSERT(raw_temperature_reg == 2400);
	TEST_ASSERT(temp == 15.);
}

static void test_get_pressure(void)
{
	float temp, pressure;
	int raw_temperature_reg;

    set_testing_module(MODULE_BMP085_TEMPERATURE);
	temp = get_temperature(&raw_temperature_reg);
	TEST_ASSERT(temp == 15.);
    set_testing_module(MODULE_BMP085_PRESSURE);
	pressure = get_pressure(raw_temperature_reg);
	TEST_ASSERT((int)(pressure * 100) == 69964);
}

static void test_get_altitude(void)
{
	float temp, pressure, altitude;
	int raw_temperature_reg;

    set_testing_module(MODULE_BMP085_TEMPERATURE);
	temp = get_temperature(&raw_temperature_reg);
	TEST_ASSERT(temp == 15.);
    set_testing_module(MODULE_BMP085_PRESSURE);
	pressure = get_pressure(raw_temperature_reg);
	TEST_ASSERT((int)(pressure * 100) == 69964);
	altitude = get_altitude(pressure, temp);
//	printf("%f\n", altitude);
	TEST_ASSERT((int)altitude == 3235);
}

TestRef run_bmp085_tests(void)
{
	EMB_UNIT_TESTFIXTURES(fixtures) {
		new_TestFixture("test_init", test_init),
		new_TestFixture("test_get_temperature", test_get_temperature),
		new_TestFixture("test_get_pressure", test_get_pressure),
		new_TestFixture("test_get_altitude", test_get_altitude),
	};

	EMB_UNIT_TESTCALLER(bmp085_test, "BMP085_Test", setUp, tearDown, fixtures);
	return (TestRef)&bmp085_test;
}
