#include <embUnit/embUnit.h>
#include "wiringPiI2C.h"
#include "../server/hmc5883l.h"

static void setUp(void)
{
    set_testing_module(MODULE_HMC5883L);
}

static void tearDown(void)
{
}

static void test_init(void)
{
    TEST_ASSERT_EQUAL_INT(-1, init_hmc5883l(-1, 0, HMC5883L_MODE_CONTINUOUS_MEASUREMENT));
    TEST_ASSERT_EQUAL_INT(-1, init_hmc5883l(0, -1, HMC5883L_MODE_CONTINUOUS_MEASUREMENT));
    TEST_ASSERT_EQUAL_INT(-1, init_hmc5883l(8,  0, HMC5883L_MODE_CONTINUOUS_MEASUREMENT));
    TEST_ASSERT_EQUAL_INT(-1, init_hmc5883l(0,  7, HMC5883L_MODE_CONTINUOUS_MEASUREMENT));
    TEST_ASSERT_EQUAL_INT(0,  init_hmc5883l(0,  0, HMC5883L_MODE_CONTINUOUS_MEASUREMENT));
    TEST_ASSERT_EQUAL_INT(0,  init_hmc5883l(7,  6, HMC5883L_MODE_SINGLE_MEASUREMENT));
}

static void test_get_heading(void)
{
    int heading;
    heading = get_heading();
    TEST_ASSERT(heading == 180);
}

TestRef run_hmc5883l_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture("test_init", test_init),
                        new_TestFixture("test_get_heading", test_get_heading),
    };
    
    EMB_UNIT_TESTCALLER(hmc5883l_test, "HMC5883L_Test", setUp, tearDown, fixtures);
    return (TestRef)&hmc5883l_test;
}

