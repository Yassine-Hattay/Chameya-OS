#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void setUp(void) {
    // This is called before each test
}

void tearDown(void) {
    // This is called after each test
}

void test_test1(void)
{
    /* Sometimes you get the test wrong.  When that happens, you get a failure too... and a quick look should tell
     * you what actually happened...which in this case was a failure to setup the initial condition. */
    TEST_ASSERT_EQUAL_HEX(0x55, 0x56);
}

void run_tests_task(void *param) {
    UNITY_BEGIN();  // Initialize Unity test framework
    RUN_TEST(test_test1);  // Example test
    UNITY_END();  // End the test execution
    vTaskDelete(NULL);  // Delete the task once tests are done
}

