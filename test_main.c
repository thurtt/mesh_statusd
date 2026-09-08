#include <stdlib.h>
#include "unity.h"

const char * RUN_CMD = "./mesh_statusd";

void setUp(void)
{
}

void tearDown(void)
{
}

void test_main_exits_zero(void)
{
    int rc = system(RUN_CMD);
    TEST_ASSERT_TRUE(rc == 0);
}

// not needed when using generate_test_runner.rb
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_main_exits_zero);
    return UNITY_END();
}
