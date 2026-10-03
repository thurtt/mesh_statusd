#include <stdlib.h>
#include "unity.h"


const char * RUN_CMD = "./mesh_statusd";

void run_display_test();
void run_measurement_tests();

int main(void)
{
    UNITY_BEGIN();
    run_display_test();
    run_measurement_tests();
    return UNITY_END();
}
