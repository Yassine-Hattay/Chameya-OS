#include "unity.h"
#include "SPI_test.h"
// Unity setup functions (empty but required)
void setUp(void) {
}
void tearDown(void) {
}

// Main function to run tests
int main(void) {
	setUp();
	UNITY_BEGIN();
	RUN_TEST(TestSPI_master_recive);
	return UNITY_END();
}
